/**
 * @file alarm_monitor.cpp
 * @brief 报警信号监控类实现
 * @details 实现报警信号的持续监控和异常上报功能
 * @author VijaySue
 * @date 2024-3-29
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
    m_alarm_map[255] = "PLC连接故障"; // 添加255特殊值表示连接故障
    
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
    
    // 添加初始延迟，确保PLC有足够时间完成初始连接和通信准备
    const int INITIAL_DELAY_MS = 500;  // 启动时等待500毫秒
    SPDLOG_INFO("等待{}毫秒让PLC通信初始化...", INITIAL_DELAY_MS);
    std::this_thread::sleep_for(std::chrono::milliseconds(INITIAL_DELAY_MS));
    
    // 记录上次PLC连接状态，用于状态变化检测
    bool last_connection_ok = true;
    
    while (m_running) {
        try {
            if (m_enabled) {
                // 获取PLCManager实例
                PLCManager& plc = PLCManager::instance();
                
                // 直接读取报警信号，无需读取所有PLC数据
                uint8_t alarm_value = plc.read_alarm_signal();
                
                // 连接状态处理
                bool current_connection_ok = (alarm_value != 255);
                
                // 处理PLC连接恢复
                if (!last_connection_ok && current_connection_ok) {
                    SPDLOG_INFO("PLC连接已恢复");
                    // 连接恢复时清除之前的255报警记录
                    auto it = m_reported_alarms.find(255);
                    if (it != m_reported_alarms.end()) {
                        m_reported_alarms.erase(it);
                    }
                }
                
                // 处理PLC连接异常
                if (!current_connection_ok) {
                    SPDLOG_ERROR("检测到PLC连接异常，尝试重新连接...");
                    // 主动触发PLC重连
                    bool reconnect_success = plc.connect_plc();
                    
                    // 只有重连也失败时才上报连接异常
                    if (!reconnect_success) {
                        // 强制上报连接异常，不等待间隔
                        report_alarm(255, "PLC连接故障", true);
                        SPDLOG_ERROR("PLC重连失败，已上报连接故障");
                    } else {
                        SPDLOG_INFO("PLC重连成功，不上报连接故障");
                        // 重连成功后，读取一次数据验证连接真正恢复
                        uint8_t verify_value = plc.read_alarm_signal();
                        if (verify_value != 255) {
                            // 连接和通信都恢复正常
                            SPDLOG_INFO("PLC连接已完全恢复");
                            current_connection_ok = true;
                        } else {
                            // 连接成功但通信仍然有问题
                            SPDLOG_ERROR("PLC连接成功但通信验证失败");
                            report_alarm(255, "PLC通信验证失败", true);
                        }
                    }
                }
                
                // 记录当前连接状态
                last_connection_ok = current_connection_ok;
                
                // 如果不是"无报警"状态且连接是正常的，则需要上报
                if (alarm_value != 16) {
                    // 如果连接状态已恢复，但alarm_value还是255，说明是过渡状态，不报告异常
                    if (alarm_value == 255 && current_connection_ok) {
                        SPDLOG_DEBUG("连接已恢复但alarm_value仍为255，等待下次检查");
                    } else {
                        std::string alarm_description = parse_alarm_signal(alarm_value);
                        
                        // 对255特殊处理，但避免重复上报
                        if (alarm_value == 255) {
                            SPDLOG_ERROR("检测到PLC连接异常: 值={}, 描述={}", alarm_value, alarm_description);
                            // 已在上面强制上报，这里不再重复上报
                        } else {
                            SPDLOG_WARN("检测到报警信号: 值={}, 描述={}", alarm_value, alarm_description);
                            // 上报非连接故障报警信号
                            report_alarm(alarm_value, alarm_description);
                        }
                    }
                } else {
                    // 如果从有报警恢复到无报警，清空已上报记录
                    if (!m_reported_alarms.empty()) {
                        SPDLOG_INFO("系统恢复到无报警状态，清除报警记录");
                        m_reported_alarms.clear();
                    }
                }
            }
        } catch (const std::exception& e) {
            SPDLOG_ERROR("报警监控异常: {}", e.what());
            
            // 发生异常时也应当上报连接故障
            report_alarm(255, "系统监控异常: " + std::string(e.what()), true);
        }
        
        // 等待指定间隔后再次检查
        std::this_thread::sleep_for(std::chrono::milliseconds(m_interval_ms));
    }
    
    SPDLOG_INFO("报警监控线程已退出");
}

std::string AlarmMonitor::parse_alarm_signal(uint8_t alarm_value) {
    // 根据报警码查找对应的报警描述
    auto it = m_alarm_map.find(alarm_value);
    if (it != m_alarm_map.end()) {
        return it->second;
    }
    
    // 如果是特殊值255但没在映射表中找到
    if (alarm_value == 255) {
        return "PLC连接故障";
    }
    
    // 如果找不到预设的报警描述，生成一个通用描述
    return "未知报警(编码:" + std::to_string(alarm_value) + ")";
}

void AlarmMonitor::report_alarm(uint8_t alarm_value, const std::string& alarm_description, bool force_report) {
    // 检查是否已经上报过该报警
    auto it = m_reported_alarms.find(alarm_value);
    
    // 获取当前时间
    auto current_time = std::chrono::steady_clock::now();
    
    // 如果已经上报过该报警，检查是否需要重新上报
    if (it != m_reported_alarms.end() && it->second.reported) {
        // 检查上次上报时间，如果超过间隔，则重新上报
        auto time_since_last_report = std::chrono::duration_cast<std::chrono::seconds>(
            current_time - it->second.last_report_time).count();
            
        // 所有报警都使用60秒的上报间隔，包括PLC连接故障(255)
        int report_interval = 60;
            
        // 如果不是强制上报且距离上次上报时间少于间隔时间，不重复上报
        if (!force_report && time_since_last_report < report_interval) {
            SPDLOG_DEBUG("报警信号已于{}秒前上报，跳过重复上报: 值={}, 描述={}", 
                time_since_last_report, alarm_value, alarm_description);
            return;
        }
        
        SPDLOG_INFO("重新上报报警信号 (已过{}秒): 值={}, 描述={}", 
            time_since_last_report, alarm_value, alarm_description);
    }
    
    // 标记该报警已上报并记录时间
    AlarmReportStatus status;
    status.reported = true;
    status.last_report_time = current_time;
    m_reported_alarms[alarm_value] = status;
    
    // 使用CallbackClient发送报警信息
    CallbackClient& client = CallbackClient::instance();
    
    try {
        // 使用专门的报警回调接口上报
        client.send_alarm_callback(alarm_description);
        
        // 对不同报警级别使用不同的日志级别
        if (alarm_value == 255) {
            SPDLOG_ERROR("已上报连接故障: 值={}, 描述={}", alarm_value, alarm_description);
        } else {
            SPDLOG_INFO("已上报报警信号: 值={}, 描述={}", alarm_value, alarm_description);
        }
    } catch (const std::exception& e) {
        SPDLOG_ERROR("上报报警信号失败: {}", e.what());
    }
} 