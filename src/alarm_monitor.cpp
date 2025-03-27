/**
 * @file alarm_monitor.cpp
 * @brief 报警信号监控类实现
 * @details 实现报警信号的持续监控和异常上报功能
 * @author VijaySue
 * @date 2024-3-27
 */

#include "../include/alarm_monitor.h"
#include "../include/plc_manager.h"
#include "../include/callback_client.h"
#include <spdlog/spdlog.h>

// 静态成员变量定义
std::thread AlarmMonitor::m_monitor_thread;
std::mutex AlarmMonitor::m_mutex;

AlarmMonitor& AlarmMonitor::instance() {
    static AlarmMonitor instance;
    return instance;
}

AlarmMonitor::AlarmMonitor() 
    : m_running(false), m_enabled(false), m_interval_ms(1000) {
    
    // 初始化报警码映射表
    m_alarm_map[0] = "油温低";
    m_alarm_map[1] = "油温高";
    m_alarm_map[2] = "液位低";
    m_alarm_map[4] = "液位高";
    m_alarm_map[8] = "滤芯堵";
    m_alarm_map[16] = "无报警";
    
    SPDLOG_INFO("报警监控初始化完成");
}

AlarmMonitor::~AlarmMonitor() {
    stop();
    SPDLOG_INFO("报警监控已销毁");
}

void AlarmMonitor::start(int interval_ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_running) {
        SPDLOG_WARN("报警监控已经在运行中");
        return;
    }
    
    m_interval_ms = interval_ms;
    m_running = true;
    m_enabled = true;
    
    // 清空已上报记录
    m_reported_alarms.clear();
    
    // 启动监控线程
    m_monitor_thread = std::thread(&AlarmMonitor::monitor_thread_func, this);
    
    SPDLOG_INFO("报警监控已启动，检查间隔: {}毫秒", m_interval_ms);
}

void AlarmMonitor::stop() {
    bool expected = true;
    if (m_running.compare_exchange_strong(expected, false)) {
        if (m_monitor_thread.joinable()) {
            m_monitor_thread.join();
        }
        SPDLOG_INFO("报警监控已停止");
    }
}

void AlarmMonitor::set_enabled(bool enabled) {
    m_enabled = enabled;
    SPDLOG_INFO("报警监控状态已设置为: {}", enabled ? "启用" : "禁用");
}

bool AlarmMonitor::is_running() const {
    return m_running;
}

void AlarmMonitor::monitor_thread_func() {
    SPDLOG_INFO("报警监控线程已启动");
    
    while (m_running) {
        try {
            if (m_enabled) {
                // 获取PLCManager实例
                PLCManager& plc = PLCManager::instance();
                
                // 直接读取报警信号，无需读取所有PLC数据
                uint8_t alarm_value = plc.read_alarm_signal();
                
                // 如果不是"无报警"状态，则需要上报
                if (alarm_value != 16) {
                    std::string alarm_description = parse_alarm_signal(alarm_value);
                    SPDLOG_WARN("检测到报警信号: 值={}, 描述={}", alarm_value, alarm_description);
                    
                    // 上报报警信号
                    report_alarm(alarm_value, alarm_description);
                } else {
                    // 如果从有报警恢复到无报警，清空已上报记录
                    if (!m_reported_alarms.empty()) {
                        SPDLOG_INFO("系统恢复到无报警状态");
                        m_reported_alarms.clear();
                    }
                }
            }
        } catch (const std::exception& e) {
            SPDLOG_ERROR("报警监控异常: {}", e.what());
        }
        
        // 等待指定间隔后再次检查
        std::this_thread::sleep_for(std::chrono::milliseconds(m_interval_ms));
    }
    
    SPDLOG_INFO("报警监控线程已退出");
}

uint8_t AlarmMonitor::check_alarm_status() {
    // 获取当前PLC状态，直接读取报警信号
    PLCManager& plc = PLCManager::instance();
    return plc.read_alarm_signal();
}

std::string AlarmMonitor::parse_alarm_signal(uint8_t alarm_value) {
    // 根据报警码查找对应的报警描述
    auto it = m_alarm_map.find(alarm_value);
    if (it != m_alarm_map.end()) {
        return it->second;
    }
    
    // 如果找不到预设的报警描述，生成一个通用描述
    return "未知报警(编码:" + std::to_string(alarm_value) + ")";
}

void AlarmMonitor::report_alarm(uint8_t alarm_value, const std::string& alarm_description) {
    // 检查是否已经上报过该报警
    auto it = m_reported_alarms.find(alarm_value);
    if (it != m_reported_alarms.end() && it->second) {
        // 已经上报过该报警，不再重复上报
        return;
    }
    
    // 标记该报警已上报
    m_reported_alarms[alarm_value] = true;
    
    // 使用CallbackClient发送报警信息
    CallbackClient& client = CallbackClient::instance();
    
    try {
        // 使用专门的报警回调接口上报
        client.send_alarm_callback(alarm_description);
        SPDLOG_INFO("已上报报警信号: 值={}, 描述={}", alarm_value, alarm_description);
    } catch (const std::exception& e) {
        SPDLOG_ERROR("上报报警信号失败: {}", e.what());
    }
} 