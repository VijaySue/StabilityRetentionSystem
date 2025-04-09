/**
 * @file alarm_monitor.h
 * @brief 报警信号监控类定义
 * @details 单独线程监控报警信号地址，检测各类报警并调用错误异常上报接口
 * @author VijaySue
 * @date 2024-3-29
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
#include <set>
#include <spdlog/spdlog.h>

/**
 * @brief 报警类型枚举
 * @details 定义不同类型的报警，用于标识报警来源
 */
enum class AlarmType {
    OIL_TEMP,       // 油温报警
    LIQUID_LEVEL,   // 液位报警
    FILTER,         // 滤芯堵报警
    CONNECTION      // PLC连接报警
};

/**
 * @brief 报警ID结构体
 * @details 用于唯一标识一个报警
 */
struct AlarmId {
    AlarmType type;    // 报警类型
    uint8_t value;     // 报警值

    // 用于哈希表比较
    bool operator==(const AlarmId& other) const {
        return type == other.type && value == other.value;
    }
    
    // 用于std::set中的比较排序
    bool operator<(const AlarmId& other) const {
        // 首先按类型排序，然后按值排序
        if (type != other.type) {
            return type < other.type;
        }
        return value < other.value;
    }
};

// 自定义哈希函数，用于AlarmId在unordered_map中的哈希
namespace std {
    template<>
    struct hash<AlarmId> {
        size_t operator()(const AlarmId& id) const {
            return hash<int>()(static_cast<int>(id.type)) ^ hash<uint8_t>()(id.value);
        }
    };
}

/**
 * @brief 报警信号监控类（单例）
 * 
 * @details 在单独线程中不间断查询报警信号地址的值，检测是否存在报警状态
 *          分别监控三个地址:
 *          VB1004(油温): 1=油温低, 2=油温高, 4=正常
 *          VB1005(液位): 1=液位低, 2=液位高, 4=正常
 *          VB1006(滤芯): 1=滤芯堵, 2=正常
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
     * @param alarm_id 报警ID
     * @param alarm_description 报警描述
     * @param is_cleared 是否是告警解除的上报
     * @param force_report 是否强制上报
     */
    void report_alarm(const AlarmId& alarm_id, const std::string& alarm_description, 
                     bool is_cleared = false, bool force_report = false);
    
    /**
     * @brief 上报PLC连接报警
     * @param force_report 是否强制上报
     */
    void report_connection_alarm(bool force_report = false);
    
    /**
     * @brief 上报PLC连接恢复
     */
    void report_connection_recovery();

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
     * @brief 解析油温报警信号
     * @param alarm_value 油温报警信号值
     * @return 报警状态描述
     */
    std::string parse_oil_temp_alarm(uint8_t alarm_value);
    
    /**
     * @brief 解析液位报警信号
     * @param alarm_value 液位报警信号值
     * @return 报警状态描述
     */
    std::string parse_liquid_level_alarm(uint8_t alarm_value);
    
    /**
     * @brief 解析滤芯报警信号
     * @param alarm_value 滤芯报警信号值
     * @return 报警状态描述
     */
    std::string parse_filter_alarm(uint8_t alarm_value);
    
    /**
     * @brief 清除特定类型的已上报报警
     * @param type 报警类型
     */
    void clear_reported_alarms_by_type(AlarmType type);
    
    /**
     * @brief 检查新的报警并上报
     * @param alarm_type 报警类型
     * @param current_value 当前报警值
     * @param normal_value 表示正常状态的值
     */
    void check_and_report_alarms(AlarmType alarm_type, uint8_t current_value, uint8_t normal_value);
    
    // 静态成员变量
    static std::thread m_monitor_thread;      // 监控线程
    static std::mutex m_mutex;                // 互斥锁，保证线程安全
    
    // 成员变量
    std::atomic<bool> m_running;              // 运行状态标志
    std::atomic<bool> m_enabled;              // 是否启用报警监控
    int m_interval_ms;                        // 检查间隔(毫秒)
    
    // 报警码映射表 - 现在分为三类
    std::unordered_map<uint8_t, std::string> m_oil_temp_alarm_map;     // 油温报警映射
    std::unordered_map<uint8_t, std::string> m_liquid_level_alarm_map; // 液位报警映射
    std::unordered_map<uint8_t, std::string> m_filter_alarm_map;       // 滤芯报警映射
    
    // 记录前一次检查的报警状态
    uint8_t m_last_oil_temp_value;      // 上次的油温报警值
    uint8_t m_last_liquid_level_value;  // 上次的液位报警值
    uint8_t m_last_filter_value;        // 上次的滤芯报警值
    bool m_last_connection_ok;          // 上次的连接状态
    
    // 已上报的报警记录，避免频繁重复上报
    std::unordered_map<AlarmId, AlarmReportStatus> m_reported_alarms;
    
    // 当前活跃的报警集合，用于跟踪哪些报警需要在恢复时上报clear状态
    std::set<AlarmId> m_active_alarms;
};