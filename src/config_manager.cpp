/**
 * @file config_manager.cpp
 * @brief 配置管理类实现
 * @details 实现配置文件的加载和解析功能
 * @author VijaySue
 * @date 2024-3-29
 */

#include "../include/config_manager.h"
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <unistd.h>
#include <linux/limits.h>
#include <iostream>

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
 * @brief 加载配置文件
 * @details 从指定路径加载INI格式的配置文件
 *          支持注释（以#开头）和空行
 *          配置文件格式：[section] key = value
 *          自动去除键值对的首尾空格
 * @param config_file 配置文件路径
 * @return 加载是否成功
 */
bool ConfigManager::load_config(const std::string& config_file) {    
    // 尝试多个可能的配置文件路径
    std::vector<std::filesystem::path> possible_paths;
    
    // 1. 直接使用传入的路径
    possible_paths.push_back(config_file);
    
    // 2. 获取可执行文件目录
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        std::filesystem::path exe_path(result);
        std::filesystem::path exe_dir = exe_path.parent_path();
        
        // 3. 在可执行文件目录下查找
        possible_paths.push_back(exe_dir / config_file);
        
        // 4. 在可执行文件的上一级目录下查找（如果在bin目录运行，这会找到项目根目录）
        possible_paths.push_back(exe_dir.parent_path() / config_file);
    }
    
    // 尝试当前工作目录
    possible_paths.push_back(std::filesystem::current_path() / config_file);
    // 尝试当前工作目录的上一级
    possible_paths.push_back(std::filesystem::current_path().parent_path() / config_file);
    
    // 尝试每一个可能的路径
    std::ifstream file;
    std::filesystem::path valid_path;
    
    for (const auto& path : possible_paths) {
        SPDLOG_INFO("尝试加载配置文件: {}", path.string());
        file.open(path);
        if (file.is_open()) {
            valid_path = path;
            SPDLOG_INFO("成功打开配置文件: {}", path.string());
            break;
        }
    }
    
    if (!file.is_open()) {
        SPDLOG_ERROR("无法打开配置文件，尝试了以下路径:");
        for (const auto& path : possible_paths) {
            SPDLOG_ERROR("  - {}", path.string());
        }
        return false;
    }
    
    std::string line;
    std::string current_section = "";
    int line_number = 0;
    
    while (std::getline(file, line)) {
        line_number++;
        
        // 去除首尾空格
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            // 空行
            continue;
        }
        line = line.substr(start);
        
        // 跳过注释
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // 处理节
        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos) {
                current_section = line.substr(1, end - 1);
                SPDLOG_DEBUG("行 {}: 找到配置节 [{}]", line_number, current_section);
                continue;
            }
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
            
            // 去除值中的注释
            size_t comment_pos = value.find('#');
            if (comment_pos != std::string::npos) {
                value = value.substr(0, comment_pos);
            }
            
            // 再次去除尾部空格（在去除注释后）
            value.erase(value.find_last_not_of(" \t") + 1);
            
            SPDLOG_DEBUG("行 {}: 读取配置项: [{}] {} = {}", line_number, current_section, key, value);
            
            // 根据节和键设置相应的值
            try {
                if (current_section == "server") {
                    if (key == "host") {
                        server_host = value;
                        SPDLOG_DEBUG("设置server.host = {}", server_host);
                    }
                    else if (key == "port") {
                        server_port = std::stoi(value);
                        SPDLOG_DEBUG("设置server.port = {}", server_port);
                    }
                }
                else if (current_section == "plc") {
                    if (key == "ip") {
                        plc_ip = value;
                        SPDLOG_DEBUG("设置plc.ip = {}", plc_ip);
                    }
                    else if (key == "port") {
                        plc_port = std::stoi(value);
                        SPDLOG_DEBUG("设置plc.port = {}", plc_port);
                    }
                }
                else if (current_section == "logging") {
                    if (key == "level") {
                        log_level = value;
                        SPDLOG_DEBUG("设置logging.level = {}", log_level);
                    }
                }
                else if (current_section == "edge_system") {
                    if (key == "url") {
                        edge_system_url = value;
                        SPDLOG_DEBUG("设置edge_system.url = {}", edge_system_url);
                    }
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
                else {
                    SPDLOG_WARN("警告: 在未知节 [{}] 中的配置项将被忽略", current_section);
                }
            } catch (const std::exception& e) {
                SPDLOG_ERROR("解析配置项时出错，行 {}: {}", line_number, e.what());
            }
        }
    }
    
    // 输出最终配置值
    SPDLOG_INFO("配置加载完成，最终配置值:");
    SPDLOG_INFO("  server.host = {}", server_host);
    SPDLOG_INFO("  server.port = {}", server_port);
    SPDLOG_INFO("  plc.ip = {}", plc_ip);
    SPDLOG_INFO("  plc.port = {}", plc_port);
    SPDLOG_INFO("  logging.level = {}", log_level);
    SPDLOG_INFO("  edge_system.url = {}", edge_system_url);
    
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
            /**
             * @brief 实现CIDR匹配
             * @details 需要添加对CIDR格式IP地址段的支持，如192.168.0.0/24
             * @note 当前版本仅支持精确IP匹配
             */
            continue;
        }
        
        if (ip == allowed_ip) {
            return true;
        }
    }
    
    return false;
} 