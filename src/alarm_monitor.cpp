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
    : m_running(false), m_enabled(false), m_interval_ms(1000),
      m_last_oil_temp_value(4), m_last_liquid_level_value(4), m_last_filter_value(2), m_last_connection_ok(true) {
    
    // 初始化油温报警码映射表
    m_oil_temp_alarm_map[1] = "油温低";
    m_oil_temp_alarm_map[2] = "油温高";
    m_oil_temp_alarm_map[4] = "油温正常";
    m_oil_temp_alarm_map[255] = "油温传感器通信故障";
    
    // 初始化液位报警码映射表
    m_liquid_level_alarm_map[1] = "液位低";
    m_liquid_level_alarm_map[2] = "液位高";
    m_liquid_level_alarm_map[4] = "液位正常";
    m_liquid_level_alarm_map[255] = "液位传感器通信故障";
    
    // 初始化滤芯报警码映射表
    m_filter_alarm_map[1] = "滤芯堵";
    m_filter_alarm_map[2] = "滤芯正常";
    m_filter_alarm_map[255] = "滤芯传感器通信故障";
    
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
    m_active_alarms.clear();
    
    // 初始化上次状态为正常值
    m_last_oil_temp_value = 4;      // 油温正常
    m_last_liquid_level_value = 4;  // 液位正常
    m_last_filter_value = 2;        // 滤芯正常
    m_last_connection_ok = true;    // 连接正常
    
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
    
    while (m_running) {
        try {
            if (m_enabled) {
                // 获取PLCManager实例
                PLCManager& plc = PLCManager::instance();
                
                // 读取报警信号，包含三个报警地址
                AlarmSignals signals = plc.read_alarm_signal();
                
                // 连接状态处理
                bool current_connection_ok = (signals.oil_temp != 255 && signals.liquid_level != 255 && signals.filter != 255);
                
                // 处理PLC连接恢复
                if (!m_last_connection_ok && current_connection_ok) {
                    SPDLOG_INFO("PLC连接已恢复");
                    report_connection_recovery();
                }
                
                // 处理PLC连接异常
                if (!current_connection_ok) {
                    SPDLOG_ERROR("检测到PLC连接异常，尝试重新连接...");
                    // 主动触发PLC重连
                    bool reconnect_success = plc.connect_plc();
                    
                    // 只有重连也失败时才上报连接异常
                    if (!reconnect_success) {
                        // 强制上报连接异常，不等待间隔
                        report_connection_alarm(true);
                        SPDLOG_ERROR("PLC重连失败，已上报连接故障");
                    } else {
                        SPDLOG_INFO("PLC重连成功，不上报连接故障");
                        // 重连成功后，读取一次数据验证连接真正恢复
                        AlarmSignals verify_signals = plc.read_alarm_signal();
                        current_connection_ok = (verify_signals.oil_temp != 255 && 
                                               verify_signals.liquid_level != 255 && 
                                               verify_signals.filter != 255);
                        
                        if (current_connection_ok) {
                            // 连接和通信都恢复正常
                            SPDLOG_INFO("PLC连接已完全恢复");
                            report_connection_recovery();
                            
                            // 使用新读取的验证数据替换原始数据
                            signals = verify_signals;
                        } else {
                            // 连接成功但通信仍然有问题
                            SPDLOG_ERROR("PLC连接成功但通信验证失败");
                            report_connection_alarm(true);
                        }
                    }
                }
                
                // 保存当前连接状态
                m_last_connection_ok = current_connection_ok;
                
                // 只有连接正常时才处理具体报警
                if (current_connection_ok) {
                    // 处理油温报警
                    check_and_report_alarms(AlarmType::OIL_TEMP, signals.oil_temp, 4);
                    
                    // 处理液位报警
                    check_and_report_alarms(AlarmType::LIQUID_LEVEL, signals.liquid_level, 4);
                    
                    // 处理滤芯报警
                    check_and_report_alarms(AlarmType::FILTER, signals.filter, 2);
                    
                    // 记录当前报警状态，用于下次比较
                    m_last_oil_temp_value = signals.oil_temp;
                    m_last_liquid_level_value = signals.liquid_level;
                    m_last_filter_value = signals.filter;
                }
            }
        } catch (const std::exception& e) {
            SPDLOG_ERROR("报警监控异常: {}", e.what());
            
            // 发生异常时也应当上报连接故障
            report_connection_alarm(true);
        }
        
        // 等待指定间隔后再次检查
        std::this_thread::sleep_for(std::chrono::milliseconds(m_interval_ms));
    }
    
    SPDLOG_INFO("报警监控线程已退出");
}

std::string AlarmMonitor::parse_oil_temp_alarm(uint8_t alarm_value) {
    auto it = m_oil_temp_alarm_map.find(alarm_value);
    if (it != m_oil_temp_alarm_map.end()) {
        return it->second;
    }
    return "未知油温报警";
}

std::string AlarmMonitor::parse_liquid_level_alarm(uint8_t alarm_value) {
    auto it = m_liquid_level_alarm_map.find(alarm_value);
    if (it != m_liquid_level_alarm_map.end()) {
        return it->second;
    }
    return "未知液位报警";
}

std::string AlarmMonitor::parse_filter_alarm(uint8_t alarm_value) {
    auto it = m_filter_alarm_map.find(alarm_value);
    if (it != m_filter_alarm_map.end()) {
        return it->second;
    }
    return "未知滤芯报警";
}

void AlarmMonitor::check_and_report_alarms(AlarmType alarm_type, uint8_t current_value, uint8_t normal_value) {
    uint8_t prev_value;
    std::string description;
    
    // 确定上一次的值和报警描述
    switch (alarm_type) {
        case AlarmType::OIL_TEMP:
            prev_value = m_last_oil_temp_value;
            description = parse_oil_temp_alarm(current_value);
            break;
        case AlarmType::LIQUID_LEVEL:
            prev_value = m_last_liquid_level_value;
            description = parse_liquid_level_alarm(current_value);
            break;
        case AlarmType::FILTER:
            prev_value = m_last_filter_value;
            description = parse_filter_alarm(current_value);
            break;
        default:
            SPDLOG_ERROR("未知报警类型");
            return;
    }
    
    // 创建报警ID
    AlarmId alarm_id = {alarm_type, current_value};
    
    // 如果当前值为正常值
    if (current_value == normal_value) {
        // 如果上次不是正常值，说明报警已解除，需要上报clear
        if (prev_value != normal_value && prev_value != 255) {
            // 从之前的非正常值转为正常值，上报清除状态
            report_alarm(AlarmId{alarm_type, prev_value}, description, true, true);
            
            // 清除该类型的活跃报警记录
            clear_reported_alarms_by_type(alarm_type);
            
            SPDLOG_INFO("报警已解除: {}", description);
        }
    } else if (current_value != 255) {  // 非正常值且非通信故障
        // 上报报警状态
        bool is_new_alarm = (prev_value != current_value);
        report_alarm(alarm_id, description, false, is_new_alarm);
        
        if (is_new_alarm) {
            SPDLOG_WARN("检测到新报警: {}", description);
        }
    }
}

void AlarmMonitor::clear_reported_alarms_by_type(AlarmType type) {
    // 查找并删除指定类型的已上报报警
    for (auto it = m_active_alarms.begin(); it != m_active_alarms.end();) {
        if (it->type == type) {
            // 删除报警记录
            auto report_it = m_reported_alarms.find(*it);
            if (report_it != m_reported_alarms.end()) {
                m_reported_alarms.erase(report_it);
            }
            // 从活跃报警集合中删除
            it = m_active_alarms.erase(it);
        } else {
            ++it;
        }
    }
}

void AlarmMonitor::report_alarm(const AlarmId& alarm_id, const std::string& alarm_description, 
                               bool is_cleared, bool force_report) {
    // 检查是否已经上报过该报警
    auto it = m_reported_alarms.find(alarm_id);
    
    // 获取当前时间
    auto current_time = std::chrono::steady_clock::now();
    
    // 如果已经上报过该报警，检查是否需要重新上报
    if (it != m_reported_alarms.end() && it->second.reported) {
        // 如果是清除报警，强制上报
        if (is_cleared) {
            force_report = true;
        } else {
            // 检查上次上报时间，如果超过间隔，则重新上报
            auto time_since_last_report = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - it->second.last_report_time).count();
                
            // 所有报警都使用60秒的上报间隔
            int report_interval = 60;
                
            // 如果不是强制上报且距离上次上报时间少于间隔时间，不重复上报
            if (!force_report && time_since_last_report < report_interval) {
                SPDLOG_DEBUG("报警信号已于{}秒前上报，跳过重复上报: {}", 
                    time_since_last_report, alarm_description);
                return;
            }
            
            SPDLOG_INFO("重新上报报警信号 (已过{}秒): {}", 
                time_since_last_report, alarm_description);
        }
    }
    
    // 标记该报警已上报并记录时间
    AlarmReportStatus status;
    status.reported = true;
    status.last_report_time = current_time;
    m_reported_alarms[alarm_id] = status;
    
    // 如果不是清除状态，将报警添加到活跃报警集合
    if (!is_cleared) {
        m_active_alarms.insert(alarm_id);
    } else {
        // 如果是清除状态，从活跃报警集合中移除
        m_active_alarms.erase(alarm_id);
    }
    
    // 使用CallbackClient发送报警信息
    CallbackClient& client = CallbackClient::instance();
    
    try {
        // 使用专门的报警回调接口上报，状态为"error"或"clear"
        std::string state = is_cleared ? "clear" : "error";
        client.send_alarm_callback(alarm_description, state);
        
        // 根据状态使用不同的日志级别
        if (is_cleared) {
            SPDLOG_INFO("已上报报警解除: [{}] {}", state, alarm_description);
        } else {
            SPDLOG_WARN("已上报报警信号: [{}] {}", state, alarm_description);
        }
    } catch (const std::exception& e) {
        SPDLOG_ERROR("上报报警信号失败: {}", e.what());
    }
}

void AlarmMonitor::report_connection_alarm(bool force_report) {
    // 创建连接报警ID
    AlarmId alarm_id = {AlarmType::CONNECTION, 1};
    report_alarm(alarm_id, "PLC连接故障", false, force_report);
}

void AlarmMonitor::report_connection_recovery() {
    // 创建连接报警ID用于清除
    AlarmId alarm_id = {AlarmType::CONNECTION, 1};
    report_alarm(alarm_id, "PLC连接已恢复", true, true);
}