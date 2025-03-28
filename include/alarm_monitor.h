/**
 * @file alarm_monitor.h
 * @brief 报警信号监控类定义
 * @details 单独线程监控报警信号地址，检测各类报警并调用错误异常上报接口
 * @author VijaySue
 * @date 2024-3-27
 */
#pragma once
#include "common.h"
#include "callback_client.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include <unordered_map>
#include <spdlog/spdlog.h>

/**
 * @brief 报警信号监控类（单例）
 * 
 * @details 在单独线程中不间断查询报警信号地址的值，检测是否存在报警状态
 *          如果值不为16则表示存在报警信号:
 *              0: 油温低
 *              1: 油温高
 *              2: 液位低
 *              4: 液位高
 *              8: 滤芯堵
 *              16: 无报警
 *          发现报警信号后调用错误异常上报接口
 */
class AlarmMonitor {
public:
    /**
     * @brief 报警上报状态结构体
     * @details 用于记录报警上报状态和时间
     */
    struct AlarmReportStatus {
        bool reported;                            // 是否已上报
        std::chrono::steady_clock::time_point last_report_time; // 上次上报时间
        
        AlarmReportStatus() : reported(false) {} // 默认构造函数
    };

    /**
     * @brief 获取AlarmMonitor单例实例
     * @return AlarmMonitor单例的引用
     */
    static AlarmMonitor& instance();
    
    /**
     * @brief 析构函数
     * @details 停止监控线程并释放资源
     */
    ~AlarmMonitor();
    
    /**
     * @brief 启动报警监控
     * @details 启动后台线程持续监控报警信号
     * @param interval_ms 检查报警信号的时间间隔(毫秒)
     */
    void start(int interval_ms = 1000);
    
    /**
     * @brief 停止报警监控
     * @details 停止后台监控线程
     */
    void stop();
    
    /**
     * @brief 设置监控是否启用
     * @param enabled 是否启用报警监控
     */
    void set_enabled(bool enabled);
    
    /**
     * @brief 检查监控是否正在运行
     * @return 是否正在运行
     */
    bool is_running() const;

    /**
     * @brief 上报报警信号
     * @param alarm_value 报警信号值
     * @param alarm_description 报警描述
     * @param force_report 是否强制上报
     */
    void report_alarm(uint8_t alarm_value, const std::string& alarm_description, bool force_report = false);

private:
    /**
     * @brief 构造函数 - 初始化监控状态
     */
    AlarmMonitor();
    
    AlarmMonitor(const AlarmMonitor&) = delete;              // 禁止拷贝构造
    AlarmMonitor& operator=(const AlarmMonitor&) = delete;   // 禁止赋值操作
    
    /**
     * @brief 监控线程主函数
     * @details 循环检查报警信号并上报异常
     */
    void monitor_thread_func();
    
    /**
     * @brief 检查报警状态
     * @details 从PLC读取报警信号，判断是否有报警
     * @return 报警状态值
     */
    uint8_t check_alarm_status();
    
    /**
     * @brief 解析报警信号
     * @param alarm_value 报警信号值
     * @return 报警状态描述
     */
    std::string parse_alarm_signal(uint8_t alarm_value);
    
    // 静态成员变量
    static std::thread m_monitor_thread;      // 监控线程
    static std::mutex m_mutex;                // 互斥锁，保证线程安全
    
    // 成员变量
    std::atomic<bool> m_running;              // 运行状态标志
    std::atomic<bool> m_enabled;              // 是否启用报警监控
    int m_interval_ms;                        // 检查间隔(毫秒)
    std::unordered_map<uint8_t, std::string> m_alarm_map; // 报警码映射表
    std::unordered_map<uint8_t, AlarmReportStatus> m_reported_alarms; // 已上报的报警记录，避免频繁重复上报
}; 