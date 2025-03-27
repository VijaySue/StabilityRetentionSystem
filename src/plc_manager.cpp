/**
 * @file plc_manager.cpp
 * @brief PLC通信管理器实现
 * @details 处理与西门子S7 PLC的Modbus TCP通信，读写PLC数据并解析状态
 * @author VijaySue
 * @version 2.0
 * @date 2024-3-11
 */
#include "../include/plc_manager.h"
#include "../include/config_manager.h"
#include <spdlog/spdlog.h>
#include <map>
#include <sstream>
#include <chrono>
#include <thread>

// 静态成员变量定义
modbus_t* PLCManager::m_modbus_ctx = nullptr;
DeviceState PLCManager::m_current_state;
std::mutex PLCManager::m_mutex;
std::thread PLCManager::m_monitor_thread;
bool PLCManager::m_running = false;

// PLC配置常量实现
std::string PLCManager::get_plc_ip() {
    return ConfigManager::instance().get_plc_ip();
}

int PLCManager::get_plc_port() {
    return ConfigManager::instance().get_plc_port();
}

/**
 * @brief 获取单例实例
 * @return PLCManager单例的引用
 */
PLCManager& PLCManager::instance() {
    static PLCManager instance;
    return instance;
}

/**
 * @brief 构造函数 - 初始化PLC连接和设备状态
 */
PLCManager::PLCManager() : m_is_connected(false) {
    // 使用初始化列表或赋值而不是memset初始化非平凡类型
    m_current_state.raw = DeviceState::RawData(); // 使用默认构造函数初始化
    
    // 尝试初始连接
    if (connect_plc()) {
        SPDLOG_INFO("PLCManager: 成功连接到PLC设备 {}:{}", get_plc_ip(), get_plc_port());
    } else {
        SPDLOG_ERROR("PLCManager: 无法连接到PLC设备 {}:{}", get_plc_ip(), get_plc_port());
    }
}

/**
 * @brief 析构函数 - 释放PLC连接
 */
PLCManager::~PLCManager() {
    // 确保断开连接并释放资源
    disconnect_plc();
    SPDLOG_INFO("PLC管理器已释放");
}

/**
 * @brief 连接到PLC设备
 * @return 成功返回true，失败返回false
 */
bool PLCManager::connect_plc() {
    // 释放之前的连接
    if (m_modbus_ctx != nullptr) {
        modbus_close(m_modbus_ctx);
        modbus_free(m_modbus_ctx);
        m_modbus_ctx = nullptr;
    }

    try {
        // 创建新的Modbus TCP连接
        m_modbus_ctx = modbus_new_tcp(get_plc_ip().c_str(), get_plc_port());
        if (m_modbus_ctx == nullptr) {
            SPDLOG_ERROR("创建Modbus连接失败: {}", modbus_strerror(errno));
            return false;
        }

        // 设置响应超时时间 - 修复参数问题
        // 原来的代码: 
        // timeval timeout;
        // timeout.tv_sec = 1;
        // timeout.tv_usec = 0;
        // modbus_set_response_timeout(m_modbus_ctx, &timeout);
        
        // 修改为直接传递秒和微秒:
        modbus_set_response_timeout(m_modbus_ctx, 1, 0); // 1秒, 0微秒

        // 建立连接
        if (modbus_connect(m_modbus_ctx) == -1) {
            SPDLOG_ERROR("连接到PLC设备失败: {}", modbus_strerror(errno));
            modbus_free(m_modbus_ctx);
            m_modbus_ctx = nullptr;
            m_is_connected = false;
            return false;
        }

        m_is_connected = true;
        return true;
    }
    catch (const std::exception& e) {
        SPDLOG_ERROR("连接PLC设备时发生异常: {}", e.what());
        if (m_modbus_ctx != nullptr) {
            modbus_free(m_modbus_ctx);
            m_modbus_ctx = nullptr;
        }
        m_is_connected = false;
        return false;
    }
}

/**
 * @brief 断开PLC连接
 */
void PLCManager::disconnect_plc() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_modbus_ctx != nullptr) {
        modbus_close(m_modbus_ctx);
        modbus_free(m_modbus_ctx);
        m_modbus_ctx = nullptr;
        m_is_connected = false;
        SPDLOG_DEBUG("已断开PLC连接");
    }
}

/**
 * @brief 获取当前设备状态
 * @details 从PLC读取最新数据，解析后返回设备状态
 * @return 当前设备状态对象
 */
DeviceState PLCManager::get_current_state() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 尝试从PLC读取数据
    if (!read_plc_data()) {
        // 读取失败，尝试重新连接
        SPDLOG_WARN("无法从PLC读取数据，尝试重新连接...");
        if (connect_plc()) {
            if (!read_plc_data()) {
                SPDLOG_ERROR("重连后仍无法从PLC读取数据");
            }
        }
    }
    
    // 将原始值解析为可读状态
    parse_raw_values();
    
    return m_current_state;
}

/**
 * @brief 从PLC读取原始数据
 * @details 使用Modbus TCP协议读取PLC的VB和VW地址数据
 * @return 成功返回true，失败返回false
 */
bool PLCManager::read_plc_data() {
    if (!m_is_connected || m_modbus_ctx == nullptr) {
        return false;
    }
    
    // 读取VB1000控制字节（包含多个位状态）
    uint8_t byte_value;
    if (modbus_read_registers(m_modbus_ctx, plc_address::VB_CONTROL_BYTE, 1, reinterpret_cast<uint16_t*>(&byte_value)) == -1) {
        SPDLOG_ERROR("读取控制字节失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVB(plc_address::VB_CONTROL_BYTE, byte_value);
    
    // 读取VB1001: 刚柔缸状态
    if (modbus_read_registers(m_modbus_ctx, plc_address::VB_CYLINDER_STATE, 1, reinterpret_cast<uint16_t*>(&byte_value)) == -1) {
        SPDLOG_ERROR("读取刚柔缸状态失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVB(plc_address::VB_CYLINDER_STATE, byte_value);
    
    // 读取VB1002: 升降平台1状态
    if (modbus_read_registers(m_modbus_ctx, plc_address::VB_LIFT_PLATFORM1, 1, reinterpret_cast<uint16_t*>(&byte_value)) == -1) {
        SPDLOG_ERROR("读取升降平台1状态失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVB(plc_address::VB_LIFT_PLATFORM1, byte_value);
    
    // 读取VB1003: 升降平台2状态
    if (modbus_read_registers(m_modbus_ctx, plc_address::VB_LIFT_PLATFORM2, 1, reinterpret_cast<uint16_t*>(&byte_value)) == -1) {
        SPDLOG_ERROR("读取升降平台2状态失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVB(plc_address::VB_LIFT_PLATFORM2, byte_value);
    
    // 读取VB1004: 报警信号
    if (modbus_read_registers(m_modbus_ctx, plc_address::VB_ALARM, 1, reinterpret_cast<uint16_t*>(&byte_value)) == -1) {
        SPDLOG_ERROR("读取报警信号失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVB(plc_address::VB_ALARM, byte_value);
    
    // 读取VD类型浮点数据（使用modbus_read_registers函数，需要转换）
    
    // 读取VD1010: 刚柔缸下降停止压力值
    uint16_t register_values[2]; // 浮点数需要读取2个寄存器
    if (modbus_read_registers(m_modbus_ctx, plc_address::VD_CYLINDER_PRESSURE, 2, register_values) == -1) {
        SPDLOG_ERROR("读取刚柔缸下降停止压力值失败: {}", modbus_strerror(errno));
        return false;
    }
    float value;
    // 从2个16位寄存器转换为32位浮点
    uint32_t combined = (register_values[0] << 16) | register_values[1];
    memcpy(&value, &combined, sizeof(float));
    m_current_state.setVD(plc_address::VD_CYLINDER_PRESSURE, value);
    
    // 读取VD1014: 升降平台上升停止压力值
    if (modbus_read_registers(m_modbus_ctx, plc_address::VD_LIFT_PRESSURE, 2, register_values) == -1) {
        SPDLOG_ERROR("读取升降平台上升停止压力值失败: {}", modbus_strerror(errno));
        return false;
    }
    combined = (register_values[0] << 16) | register_values[1];
    memcpy(&value, &combined, sizeof(float));
    m_current_state.setVD(plc_address::VD_LIFT_PRESSURE, value);
    
    // 读取VD1018: 平台1倾斜角度
    if (modbus_read_registers(m_modbus_ctx, plc_address::VD_PLATFORM1_TILT, 2, register_values) == -1) {
        SPDLOG_ERROR("读取平台1倾斜角度失败: {}", modbus_strerror(errno));
        return false;
    }
    combined = (register_values[0] << 16) | register_values[1];
    memcpy(&value, &combined, sizeof(float));
    m_current_state.setVD(plc_address::VD_PLATFORM1_TILT, value);
    
    // 读取VD1022: 平台2倾斜角度
    if (modbus_read_registers(m_modbus_ctx, plc_address::VD_PLATFORM2_TILT, 2, register_values) == -1) {
        SPDLOG_ERROR("读取平台2倾斜角度失败: {}", modbus_strerror(errno));
        return false;
    }
    combined = (register_values[0] << 16) | register_values[1];
    memcpy(&value, &combined, sizeof(float));
    m_current_state.setVD(plc_address::VD_PLATFORM2_TILT, value);
    
    // 读取VD1026: 平台1位置信息
    if (modbus_read_registers(m_modbus_ctx, plc_address::VD_PLATFORM1_POS, 2, register_values) == -1) {
        SPDLOG_ERROR("读取平台1位置信息失败: {}", modbus_strerror(errno));
        return false;
    }
    combined = (register_values[0] << 16) | register_values[1];
    memcpy(&value, &combined, sizeof(float));
    m_current_state.setVD(plc_address::VD_PLATFORM1_POS, value);
    
    // 读取VD1030: 平台2位置信息
    if (modbus_read_registers(m_modbus_ctx, plc_address::VD_PLATFORM2_POS, 2, register_values) == -1) {
        SPDLOG_ERROR("读取平台2位置信息失败: {}", modbus_strerror(errno));
        return false;
    }
    combined = (register_values[0] << 16) | register_values[1];
    memcpy(&value, &combined, sizeof(float));
    m_current_state.setVD(plc_address::VD_PLATFORM2_POS, value);
    
    // 解析所有原始数据
    parse_raw_values();
    
    return true;
}

/**
 * @brief 解析PLC原始数据为可读状态
 * @details 将读取的原始VB/VW数据解析为人类可读的状态文本和数值
 */
void PLCManager::parse_raw_values() {
    // 解析VB1000控制字节中的位
    // 解析操作模式 - 位0
    bool mode_bit = m_current_state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_OPERATION_MODE);
    m_current_state.operationMode = mode_bit ? "自动" : "手动";
    
    // 解析急停状态 - 位1
    bool emergency_bit = m_current_state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_EMERGENCY_STOP);
    m_current_state.emergencyStop = emergency_bit ? "正常" : "急停";
    
    // 解析油泵状态 - 位2
    bool pump_bit = m_current_state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_OIL_PUMP);
    m_current_state.oilPumpStatus = pump_bit ? "启动" : "停止";
    
    // 解析电加热状态 - 位3
    bool heater_bit = m_current_state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_HEATER);
    m_current_state.heaterStatus = heater_bit ? "加热" : "停止";
    
    // 解析风冷状态 - 位4
    bool cooling_bit = m_current_state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_AIR_COOLING);
    m_current_state.coolingStatus = cooling_bit ? "启动" : "停止";
    
    // 解析1#电动缸调平 - 位5
    bool leveling1_bit = m_current_state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_LEVELING1);
    m_current_state.leveling1Status = leveling1_bit ? "启动" : "停止";
    
    // 解析2#电动缸调平 - 位6
    bool leveling2_bit = m_current_state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_LEVELING2);
    m_current_state.leveling2Status = leveling2_bit ? "启动" : "停止";
    
    // 解析VB1001: 刚柔缸状态
    uint8_t cylinder = m_current_state.getVB(plc_address::VB_CYLINDER_STATE);
    switch (cylinder) {
        case 1: m_current_state.cylinderState = "下降停止"; break;
        case 2: m_current_state.cylinderState = "下降加压"; break;
        case 4: m_current_state.cylinderState = "上升停止"; break;
        case 8: m_current_state.cylinderState = "上升加压"; break;
        default: m_current_state.cylinderState = "未知状态";
    }
    
    // 解析VB1002: 升降平台1状态
    uint8_t platform1 = m_current_state.getVB(plc_address::VB_LIFT_PLATFORM1);
    switch (platform1) {
        case 1: m_current_state.platform1State = "上升"; break;
        case 2: m_current_state.platform1State = "上升停止"; break;
        case 4: m_current_state.platform1State = "下降"; break;
        case 8: m_current_state.platform1State = "下降停止"; break;
        default: m_current_state.platform1State = "未知状态";
    }
    
    // 解析VB1003: 升降平台2状态
    uint8_t platform2 = m_current_state.getVB(plc_address::VB_LIFT_PLATFORM2);
    switch (platform2) {
        case 1: m_current_state.platform2State = "上升"; break;
        case 2: m_current_state.platform2State = "上升停止"; break;
        case 4: m_current_state.platform2State = "下降"; break;
        case 8: m_current_state.platform2State = "下降停止"; break;
        default: m_current_state.platform2State = "未知状态";
    }
    
    // 解析VB1004: 报警信号
    uint8_t alarm = m_current_state.getVB(plc_address::VB_ALARM);
    
    if (alarm == 0) {
        m_current_state.alarmStatus = "油温低";
    } else if (alarm & 0x01) {
        m_current_state.alarmStatus = "油温高";
    } else if (alarm & 0x02) {
        m_current_state.alarmStatus = "液位低";
    } else if (alarm & 0x04) {
        m_current_state.alarmStatus = "液位高";
    } else if (alarm & 0x08) {
        m_current_state.alarmStatus = "滤芯堵";
    } else {
        m_current_state.alarmStatus = "未知报警";
    }
    
    // 解析VD1010: 刚柔缸下降停止压力值 (float)
    m_current_state.cylinderPressure = m_current_state.getVD(plc_address::VD_CYLINDER_PRESSURE);
    
    // 解析VD1014: 升降平台上升停止压力值 (float)
    m_current_state.liftPressure = m_current_state.getVD(plc_address::VD_LIFT_PRESSURE);
    
    // 解析VD1018: 平台1倾斜角度 (float)
    m_current_state.platform1TiltAngle = m_current_state.getVD(plc_address::VD_PLATFORM1_TILT);
    
    // 解析VD1022: 平台2倾斜角度 (float)
    m_current_state.platform2TiltAngle = m_current_state.getVD(plc_address::VD_PLATFORM2_TILT);
    
    // 解析VD1026: 平台1位置信息 (float)
    m_current_state.platform1Position = m_current_state.getVD(plc_address::VD_PLATFORM1_POS);
    
    // 解析VD1030: 平台2位置信息 (float)
    m_current_state.platform2Position = m_current_state.getVD(plc_address::VD_PLATFORM2_POS);
}

/**
 * @brief 执行高级业务操作
 * @details 将业务层面的操作转换为PLC层面的命令并执行
 * @param operation 操作指令
 * @return 操作是否成功执行
 */
bool PLCManager::execute_operation(const std::string& operation) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_is_connected || m_modbus_ctx == nullptr) {
        SPDLOG_ERROR("PLC未连接，无法执行操作: {}", operation);
        return false;
    }
    
    int result = -1;
    
    // 仅保留M地址操作
    if (operation == "刚性支撑") {  // JSON请求中state="刚性支撑"
        // 对应M22.1
        result = modbus_write_bit(m_modbus_ctx, 22*8 + 1, 1);
        SPDLOG_DEBUG("执行刚性支撑命令，写入M22.1=1");
    }
    else if (operation == "柔性复位") {  // JSON请求中state="柔性复位"
        // 对应M22.2
        result = modbus_write_bit(m_modbus_ctx, 22*8 + 2, 1);
        SPDLOG_DEBUG("执行柔性复位命令，写入M22.2=1");
    }
    else if (operation == "平台1上升" || operation == "平台1升高") {  // JSON请求中state="升高"，platformNum=1
        // 对应M22.3
        result = modbus_write_bit(m_modbus_ctx, 22*8 + 3, 1);
        SPDLOG_DEBUG("执行平台1上升命令，写入M22.3=1");
    }
    else if (operation == "平台1下降" || operation == "平台1复位") {  // JSON请求中state="复位"，platformNum=1
        // 对应M22.4
        result = modbus_write_bit(m_modbus_ctx, 22*8 + 4, 1);
        SPDLOG_DEBUG("执行平台1复位命令，写入M22.4=1");
    }
    else if (operation == "平台2上升" || operation == "平台2升高") {  // JSON请求中state="升高"，platformNum=2
        // 对应M22.5
        result = modbus_write_bit(m_modbus_ctx, 22*8 + 5, 1);
        SPDLOG_DEBUG("执行平台2上升命令，写入M22.5=1");
    }
    else if (operation == "平台2下降" || operation == "平台2复位") {  // JSON请求中state="复位"，platformNum=2
        // 对应M22.6
        result = modbus_write_bit(m_modbus_ctx, 22*8 + 6, 1);
        SPDLOG_DEBUG("执行平台2复位命令，写入M22.6=1");
    }
    else if (operation == "平台1调平" || operation == "1号平台调平") {  // JSON请求中state="调平"，platformNum=1
        // 对应M22.7
        result = modbus_write_bit(m_modbus_ctx, 22*8 + 7, 1);
        SPDLOG_DEBUG("执行平台1调平命令，写入M22.7=1");
    }
    else if (operation == "平台1调平复位" || operation == "1号平台调平复位") {  // JSON请求中state="调平复位"，platformNum=1
        // 对应M23.0
        result = modbus_write_bit(m_modbus_ctx, 23*8 + 0, 1);
        SPDLOG_DEBUG("执行平台1调平复位命令，写入M23.0=1");
    }
    else if (operation == "平台2调平" || operation == "2号平台调平") {  // JSON请求中state="调平"，platformNum=2
        // 对应M23.1
        result = modbus_write_bit(m_modbus_ctx, 23*8 + 1, 1);
        SPDLOG_DEBUG("执行平台2调平命令，写入M23.1=1");
    }
    else if (operation == "平台2调平复位" || operation == "2号平台调平复位") {  // JSON请求中state="调平复位"，platformNum=2
        // 对应M23.2
        result = modbus_write_bit(m_modbus_ctx, 23*8 + 2, 1);
        SPDLOG_DEBUG("执行平台2调平复位命令，写入M23.2=1");
    }
    else {
        SPDLOG_WARN("未实现的PLC操作: {}", operation);
        return false;
    }
    
    if (result == -1) {
        SPDLOG_ERROR("执行操作失败: {} (错误: {})", operation, modbus_strerror(errno));
        return false;  // 操作失败，返回false
    } else {
        SPDLOG_INFO("成功执行操作: {}", operation);
        return true;   // 操作成功，返回true
    }
}
