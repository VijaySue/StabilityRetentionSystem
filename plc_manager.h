// plc_manager.h
#pragma once
#include "common.h"
#include <mutex>
#include <modbus/modbus.h>  // 添加libmodbus库支持
#include <string>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>

/**
 * @brief PLC通信管理类（单例）
 * 
 * @details 负责与PLC设备进行通信，实现Modbus TCP协议的数据读写操作
 *          支持稳定性保持系统对设备的监控和控制功能
 *          实现了自动重连和错误处理机制
 * @note 使用libmodbus 3.1.10库实现通信功能
 */
class PLCManager {
public:
    /**
     * @brief 获取PLCManager单例实例
     * @return PLCManager单例的引用
     */
    static PLCManager& instance();
    
    /**
     * @brief 析构函数
     * @details 释放PLC连接资源
     */
    ~PLCManager();
    
    /**
     * @brief 连接到PLC设备
     * @return 连接是否成功
     */
    bool connect_plc();
    
    /**
     * @brief 断开PLC连接
     */
    void disconnect_plc();
    
    /**
     * @brief 获取当前设备状态
     * @details 从PLC读取所有设备状态数据，用于实时状态获取接口
     * @return 设备状态结构体，包含完整的设备状态信息
     */
    DeviceState get_current_state();
    
    /**
     * @brief 向PLC写入数据
     * @details 解析命令并写入对应的PLC地址
     * @param cmd 写入命令
     * @return 写入是否成功
     */
    bool write_plc_data(const std::string& cmd);
    
    /**
     * @brief 从PLC读取数据
     * @details 读取所有PLC地址的原始数据
     * @return 读取是否成功
     */
    bool read_plc_data();
    
    /**
     * @brief 解析原始值
     * @details 将原始PLC数据转换为可读的设备状态信息
     */
    void parse_raw_values();
    
    /**
     * @brief 执行高级业务操作
     * @details 将业务层面的操作转换为PLC层面的命令并执行
     *          支持刚性支撑、柔性复位、升降平台控制等操作
     * @param operation 操作指令，如"刚性支撑"、"柔性复位"等
     */
    void execute_operation(const std::string& operation);

    // PLC设备配置常量
    static constexpr const char* PLC_IP_ADDRESS = "192.168.1.10"; // PLC的IP地址
    static constexpr int PLC_PORT = 502;                          // Modbus TCP默认端口

private:
    /**
     * @brief 构造函数 - 初始化PLC连接和设备状态
     */
    PLCManager();
    
    PLCManager(const PLCManager&) = delete;              // 禁止拷贝构造
    PLCManager& operator=(const PLCManager&) = delete;   // 禁止赋值操作
    
    modbus_t* m_modbus_ctx;    // Modbus上下文
    bool m_is_connected;       // 连接状态
    std::mutex m_mutex;        // 互斥锁，保证线程安全
    DeviceState m_current_state; // 当前设备状态
};