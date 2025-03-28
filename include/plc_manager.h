/**
 * @file plc_manager.h
 * @brief PLC通信管理类定义
 * @details 负责与PLC设备进行通信，实现西门子S7协议的数据读写操作
 * @author VijaySue
 * @date 2024-3-11
 */
#pragma once
#include "common.h"
#include <mutex>
#include <string>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>
#include <snap7.h>  // 添加Snap7库支持

/**
 * @brief PLC通信管理类（单例）
 * 
 * @details 负责与PLC设备进行通信，实现西门子S7协议的数据读写操作
 *          支持稳定性保持系统对设备的监控和控制功能
 *          实现了自动重连和错误处理机制
 * @note 使用Snap7库实现与西门子PLC的通信功能
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
     * @return 操作是否成功执行
     */
    bool execute_operation(const std::string& operation);

    /**
     * @brief 从PLC读取报警信号
     * @details 仅读取报警信号地址(VB_ALARM)的数据，用于报警监控
     * @return 读取的报警信号值，如果读取失败返回255
     */
    uint8_t read_alarm_signal();

    /**
     * @brief 获取PLC连接状态
     * @return 如果PLC已连接返回true，否则返回false
     */
    bool is_connected() const { return m_is_connected; }

    // PLC设备配置常量
    static std::string get_plc_ip();  // PLC的IP地址
    static int get_plc_port();        // Modbus TCP默认端口，对于Snap7为102

private:
    /**
     * @brief 构造函数 - 初始化PLC连接和设备状态
     */
    PLCManager();
    
    PLCManager(const PLCManager&) = delete;              // 禁止拷贝构造
    PLCManager& operator=(const PLCManager&) = delete;   // 禁止赋值操作
    
    // 静态成员变量
    static TS7Client* m_client;    // Snap7客户端对象
    static DeviceState m_current_state; // 当前设备状态
    static std::mutex m_mutex;        // 互斥锁，保证线程安全
    static std::thread m_monitor_thread; // 监控线程
    static bool m_running;            // 运行状态标志
    
    bool m_is_connected;       // 连接状态
}; 