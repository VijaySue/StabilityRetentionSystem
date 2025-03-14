/**
 * @file config_manager.cpp
 * @brief 配置管理类实现
 * @details 实现配置文件的加载和解析功能
 * @author VijaySue
 * @date 2024-3-11
 */

#include "../include/config_manager.h"
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <unistd.h>
#include <linux/limits.h>

/**
 * @brief 获取ConfigManager单例实例
 * @details 使用静态局部变量实现线程安全的单例模式
 * @return ConfigManager单例的引用
 */
ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

/**
 * @brief 获取可执行文件的目录路径
 * @return 可执行文件所在目录的路径
 */
std::filesystem::path get_executable_path() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    std::filesystem::path exe_path(count != -1 ? result : std::filesystem::current_path());
    return exe_path.parent_path();
}

/**
 * @brief 加载配置文件
 * @details 从指定路径加载INI格式的配置文件
 *          支持注释（以#开头）和空行
 *          配置文件格式：[section] key = value
 *          自动去除键值对的首尾空格
 * @param config_file 配置文件路径
 * @return 加载是否成功
 */
bool ConfigManager::load_config(const std::string& config_file) {
    // 获取可执行文件所在目录
    std::filesystem::path exe_dir = get_executable_path();
    std::filesystem::path config_path = exe_dir / config_file;
    
    // 如果文件不存在，尝试在上级目录的config文件夹中查找
    if (!std::filesystem::exists(config_path)) {
        config_path = exe_dir.parent_path() / "config" / "config.ini";
    }
    
    std::ifstream file(config_path);
    if (!file.is_open()) {
        SPDLOG_ERROR("无法打开配置文件: {}", config_path.string());
        return false;
    }

    std::string line;
    std::string current_section;
    
    while (std::getline(file, line)) {
        // 跳过空行和注释
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // 处理节
        if (line[0] == '[' && line[line.length() - 1] == ']') {
            current_section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // 处理键值对
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除首尾空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // 根据节和键设置相应的值
            if (current_section == "server") {
                if (key == "host") server_host = value;
                else if (key == "port") server_port = std::stoi(value);
            }
            else if (current_section == "plc") {
                if (key == "ip") plc_ip = value;
                else if (key == "port") plc_port = std::stoi(value);
            }
            else if (current_section == "logging") {
                if (key == "level") log_level = value;
            }
            else if (current_section == "security") {
                if (key == "basic_auth") basic_auth_enabled_ = (value == "true" || value == "1");
                else if (key == "username") username_ = value;
                else if (key == "password") password_ = value;
                else if (key == "ip_whitelist") ip_whitelist_enabled_ = (value == "true" || value == "1");
                else if (key == "allowed_ips") {
                    // 解析IP白名单
                    std::istringstream iss(value);
                    std::string ip;
                    allowed_ips_.clear();
                    while (std::getline(iss, ip, ',')) {
                        // 去除空格
                        ip.erase(0, ip.find_first_not_of(" "));
                        ip.erase(ip.find_last_not_of(" ") + 1);
                        if (!ip.empty()) {
                            allowed_ips_.push_back(ip);
                        }
                    }
                }
            }
        }
    }
    
    // 如果没有找到security节，使用默认安全配置
    if (current_section != "security") {
        basic_auth_enabled_ = false;
        ip_whitelist_enabled_ = false;
    }
    
    SPDLOG_INFO("成功加载配置文件: {}", config_path.string());
    return true;
}

bool ConfigManager::is_ip_allowed(const std::string& ip) const {
    if (!ip_whitelist_enabled_) {
        return true;  // 如果未启用白名单，允许所有IP
    }
    
    // 检查IP是否在白名单中
    for (const auto& allowed_ip : allowed_ips_) {
        // 支持CIDR格式的IP地址
        if (allowed_ip.find('/') != std::string::npos) {
            // TODO: 实现CIDR匹配
            // 这里需要添加CIDR匹配的实现
            continue;
        }
        
        if (ip == allowed_ip) {
            return true;
        }
    }
    
    return false;
} 