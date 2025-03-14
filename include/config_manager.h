/**
 * @file config_manager.h
 * @brief 配置管理类定义
 * @details 负责加载和管理系统配置文件，提供统一的配置访问接口
 * @author VijaySue
 * @date 2024-3-11
 */

#pragma once

#include <string>
#include <memory>
#include <cpprest/json.h>

/**
 * @brief 配置管理类（单例）
 * 
 * @details 负责加载和管理系统配置文件，提供统一的配置访问接口
 *          支持从INI格式的配置文件中读取系统配置
 *          实现了单例模式，确保全局只有一个配置管理实例
 *          提供了默认配置值，确保在配置文件缺失时系统仍能正常运行
 */
class ConfigManager {
public:
    /**
     * @brief 获取ConfigManager单例实例
     * @return ConfigManager单例的引用
     */
    static ConfigManager& instance();
    
    /**
     * @brief 禁止拷贝构造
     */
    ConfigManager(const ConfigManager&) = delete;
    
    /**
     * @brief 禁止赋值操作
     */
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    /**
     * @brief 加载配置文件
     * @details 从指定路径加载INI格式的配置文件
     *          支持注释（以#开头）和空行
     *          配置文件格式：[section] key = value
     * @param config_file 配置文件路径
     * @return 加载是否成功
     */
    bool load_config(const std::string& config_file);
    
    /**
     * @brief 获取服务器主机地址
     * @return 服务器监听地址
     */
    std::string get_server_host() const { return server_host; }
    
    /**
     * @brief 获取服务器端口
     * @return 服务器监听端口
     */
    int get_server_port() const { return server_port; }
    
    /**
     * @brief 获取PLC设备IP地址
     * @return PLC设备的IP地址
     */
    std::string get_plc_ip() const { return plc_ip; }
    
    /**
     * @brief 获取PLC设备端口
     * @return PLC设备的Modbus TCP端口
     */
    int get_plc_port() const { return plc_port; }
    
    /**
     * @brief 获取日志级别
     * @return 日志级别字符串
     */
    std::string get_log_level() const { return log_level; }

private:
    /**
     * @brief 私有构造函数
     * @details 初始化默认配置值
     */
    ConfigManager() = default;
    
    // 服务器配置
    std::string server_host = "0.0.0.0";  // 默认监听所有接口
    int server_port = 8080;               // 默认端口
    
    // PLC配置
    std::string plc_ip = "192.168.1.10";  // 默认PLC IP地址
    int plc_port = 502;                   // 默认Modbus TCP端口
    
    // 日志配置
    std::string log_level = "info";       // 默认日志级别
}; 