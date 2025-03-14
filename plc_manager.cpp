/**
 * @file plc_manager.cpp
 * @brief PLC通信管理器实现
 * @details 处理与西门子S7 PLC的Modbus TCP通信，读写PLC数据并解析状态
 * @author Stability Retention System Team
 * @version 1.0
 * @date 2023-06-01
 */
#include "plc_manager.h"
#include <spdlog/spdlog.h>
#include <map>
#include <sstream>
#include <chrono>
#include <thread>

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
PLCManager::PLCManager() : m_modbus_ctx(nullptr), m_is_connected(false) {
    // 使用初始化列表或赋值而不是memset初始化非平凡类型
    // 原来的代码: memset(&m_current_state.raw, 0, sizeof(m_current_state.raw));
    m_current_state.raw = DeviceState::RawData(); // 使用默认构造函数初始化
    
    // 尝试初始连接
    if (connect_plc()) {
        SPDLOG_INFO("PLCManager: 成功连接到PLC设备 {}:{}", PLC_IP_ADDRESS, PLC_PORT);
    } else {
        SPDLOG_ERROR("PLCManager: 无法连接到PLC设备 {}:{}", PLC_IP_ADDRESS, PLC_PORT);
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
        m_modbus_ctx = modbus_new_tcp(PLC_IP_ADDRESS, PLC_PORT);
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
    
    // 读取VB类型数据（使用modbus_read_bits函数）
    
    // 读取VB1000: 操作模式
    uint8_t bit_values[8];
    if (modbus_read_bits(m_modbus_ctx, plc_address::VB_OPERATION_MODE, 8, bit_values) == -1) {
        SPDLOG_ERROR("读取操作模式失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVB(plc_address::VB_OPERATION_MODE, bit_values[0]);
    
    // 读取VB1001: 急停按钮
    if (modbus_read_bits(m_modbus_ctx, plc_address::VB_EMERGENCY_STOP, 8, bit_values) == -1) {
        SPDLOG_ERROR("读取急停按钮状态失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVB(plc_address::VB_EMERGENCY_STOP, bit_values[0]);
    
    // 读取VB1002: 油泵状态
    if (modbus_read_bits(m_modbus_ctx, plc_address::VB_OIL_PUMP, 8, bit_values) == -1) {
        SPDLOG_ERROR("读取油泵状态失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVB(plc_address::VB_OIL_PUMP, bit_values[0]);
    
    // 读取VB1003: 刚柔缸状态
    if (modbus_read_bits(m_modbus_ctx, plc_address::VB_CYLINDER_STATE, 8, bit_values) == -1) {
        SPDLOG_ERROR("读取刚柔缸状态失败: {}", modbus_strerror(errno));
        return false;
    }
    
    // 根据位状态确定刚柔缸状态
    uint8_t cylinder_state = 0;
    for (int i = 0; i < 8; i++) {
        if (bit_values[i]) {
            cylinder_state |= (1 << i);
        }
    }
    m_current_state.setVB(plc_address::VB_CYLINDER_STATE, cylinder_state);
    
    // 读取VB1004: 升降平台1状态
    if (modbus_read_bits(m_modbus_ctx, plc_address::VB_LIFT_PLATFORM1, 8, bit_values) == -1) {
        SPDLOG_ERROR("读取升降平台1状态失败: {}", modbus_strerror(errno));
        return false;
    }
    
    // 根据位状态确定升降平台1状态
    uint8_t platform1_state = 0;
    for (int i = 0; i < 8; i++) {
        if (bit_values[i]) {
            platform1_state |= (1 << i);
        }
    }
    m_current_state.setVB(plc_address::VB_LIFT_PLATFORM1, platform1_state);
    
    // 读取VB1005: 升降平台2状态
    if (modbus_read_bits(m_modbus_ctx, plc_address::VB_LIFT_PLATFORM2, 8, bit_values) == -1) {
        SPDLOG_ERROR("读取升降平台2状态失败: {}", modbus_strerror(errno));
        return false;
    }
    
    // 根据位状态确定升降平台2状态
    uint8_t platform2_state = 0;
    for (int i = 0; i < 8; i++) {
        if (bit_values[i]) {
            platform2_state |= (1 << i);
        }
    }
    m_current_state.setVB(plc_address::VB_LIFT_PLATFORM2, platform2_state);
    
    // 读取VB1006: 电加热状态
    if (modbus_read_bits(m_modbus_ctx, plc_address::VB_HEATER, 8, bit_values) == -1) {
        SPDLOG_ERROR("读取电加热状态失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVB(plc_address::VB_HEATER, bit_values[0]);
    
    // 读取VB1007: 风冷状态
    if (modbus_read_bits(m_modbus_ctx, plc_address::VB_AIR_COOLING, 8, bit_values) == -1) {
        SPDLOG_ERROR("读取风冷状态失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVB(plc_address::VB_AIR_COOLING, bit_values[0]);
    
    // 读取VB1008: 报警信号
    if (modbus_read_bits(m_modbus_ctx, plc_address::VB_ALARM, 8, bit_values) == -1) {
        SPDLOG_ERROR("读取报警信号失败: {}", modbus_strerror(errno));
        return false;
    }
    
    // 报警信号使用5个位来表示
    uint8_t alarm_state = 0;
    for (int i = 0; i < 5; i++) {
        if (bit_values[i]) {
            alarm_state |= (1 << i);
        }
    }
    m_current_state.setVB(plc_address::VB_ALARM, alarm_state);
    
    // 读取VB1009: 电动缸调平
    if (modbus_read_bits(m_modbus_ctx, plc_address::VB_LEVELING, 8, bit_values) == -1) {
        SPDLOG_ERROR("读取电动缸调平状态失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVB(plc_address::VB_LEVELING, bit_values[0]);
    
    // 读取VW类型数据（使用modbus_read_registers函数）
    
    // 读取VW100: 刚柔缸下降停止压力值
    uint16_t register_values[1];
    if (modbus_read_registers(m_modbus_ctx, plc_address::VW_CYLINDER_PRESSURE, 1, register_values) == -1) {
        SPDLOG_ERROR("读取刚柔缸下降停止压力值失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVW(plc_address::VW_CYLINDER_PRESSURE, static_cast<int16_t>(register_values[0]));
    
    // 读取VW104: 升降平台1上升停止压力值
    if (modbus_read_registers(m_modbus_ctx, plc_address::VW_PLATFORM1_PRESSURE, 1, register_values) == -1) {
        SPDLOG_ERROR("读取升降平台1上升停止压力值失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVW(plc_address::VW_PLATFORM1_PRESSURE, static_cast<int16_t>(register_values[0]));
    
    // 读取VW108: 升降平台2上升停止压力值
    if (modbus_read_registers(m_modbus_ctx, plc_address::VW_PLATFORM2_PRESSURE, 1, register_values) == -1) {
        SPDLOG_ERROR("读取升降平台2上升停止压力值失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVW(plc_address::VW_PLATFORM2_PRESSURE, static_cast<int16_t>(register_values[0]));
    
    // 读取VW112: 平台倾斜角度
    if (modbus_read_registers(m_modbus_ctx, plc_address::VW_TILT_ANGLE, 1, register_values) == -1) {
        SPDLOG_ERROR("读取平台倾斜角度失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVW(plc_address::VW_TILT_ANGLE, static_cast<int16_t>(register_values[0]));
    
    // 读取VW116: 平台位置信息
    if (modbus_read_registers(m_modbus_ctx, plc_address::VW_POSITION, 1, register_values) == -1) {
        SPDLOG_ERROR("读取平台位置信息失败: {}", modbus_strerror(errno));
        return false;
    }
    m_current_state.setVW(plc_address::VW_POSITION, static_cast<int16_t>(register_values[0]));
    
    return true;
}

/**
 * @brief 解析PLC原始数据为可读状态
 * @details 将读取的原始VB/VW数据解析为人类可读的状态文本和数值
 */
void PLCManager::parse_raw_values() {
    // 解析VB1000: 操作模式
    uint8_t mode = m_current_state.getVB(plc_address::VB_OPERATION_MODE);
    m_current_state.operationMode = (mode == 1) ? "手动" : (mode == 2) ? "自动" : "未知";
    
    // 解析VB1001: 急停按钮
    uint8_t emergency = m_current_state.getVB(plc_address::VB_EMERGENCY_STOP);
    m_current_state.emergencyStop = (emergency == 1) ? "复位" : (emergency == 2) ? "急停" : "未知";
    
    // 解析VB1002: 油泵状态
    uint8_t pump = m_current_state.getVB(plc_address::VB_OIL_PUMP);
    m_current_state.oilPumpStatus = (pump == 1) ? "停止" : (pump == 2) ? "启动" : "未知";
    
    // 解析VB1003: 刚柔缸状态
    uint8_t cylinder = m_current_state.getVB(plc_address::VB_CYLINDER_STATE);
    switch (cylinder) {
        case 1: m_current_state.cylinderState = "下降停止"; break;
        case 2: m_current_state.cylinderState = "下降加压"; break;
        case 4: m_current_state.cylinderState = "上升停止"; break;
        case 8: m_current_state.cylinderState = "上升停止"; break;
        default: m_current_state.cylinderState = "未知状态";
    }
    
    // 解析VB1004: 升降平台1状态
    uint8_t platform1 = m_current_state.getVB(plc_address::VB_LIFT_PLATFORM1);
    switch (platform1) {
        case 1: m_current_state.platform1State = "上升"; break;
        case 2: m_current_state.platform1State = "上升停止"; break;
        case 4: m_current_state.platform1State = "下降"; break;
        case 8: m_current_state.platform1State = "下降停止"; break;
        default: m_current_state.platform1State = "未知状态";
    }
    
    // 解析VB1005: 升降平台2状态
    uint8_t platform2 = m_current_state.getVB(plc_address::VB_LIFT_PLATFORM2);
    switch (platform2) {
        case 1: m_current_state.platform2State = "上升"; break;
        case 2: m_current_state.platform2State = "上升停止"; break;
        case 4: m_current_state.platform2State = "下降"; break;
        case 8: m_current_state.platform2State = "下降停止"; break;
        default: m_current_state.platform2State = "未知状态";
    }
    
    // 解析VB1006: 电加热状态
    uint8_t heater = m_current_state.getVB(plc_address::VB_HEATER);
    m_current_state.heaterStatus = (heater == 1) ? "停止" : (heater == 2) ? "启动" : "未知";
    
    // 解析VB1007: 风冷状态
    uint8_t cooling = m_current_state.getVB(plc_address::VB_AIR_COOLING);
    m_current_state.coolingStatus = (cooling == 1) ? "停止" : (cooling == 2) ? "启动" : "未知";
    
    // 解析VB1008: 报警信号
    uint8_t alarm = m_current_state.getVB(plc_address::VB_ALARM);
    std::string alarm_details;
    if (alarm & 0x01) alarm_details += "油温高 ";
    if (alarm & 0x02) alarm_details += "液位低 ";
    if (alarm & 0x04) alarm_details += "液位高 ";
    if (alarm & 0x08) alarm_details += "滤芯堵 ";
    
    m_current_state.alarmStatus = alarm_details.empty() ? "油温低" : alarm_details;
    
    // 解析VB1009: 电动缸调平
    uint8_t leveling = m_current_state.getVB(plc_address::VB_LEVELING);
    m_current_state.levelingStatus = (leveling == 1) ? "停止" : (leveling == 2) ? "启动" : "未知";
    
    // 解析VW100: 刚柔缸下降停止压力值
    m_current_state.cylinderPressure = m_current_state.getVW(plc_address::VW_CYLINDER_PRESSURE) / 100.0; // 假设原始值需要除以100
    
    // 解析VW104: 升降平台1上升停止压力值
    m_current_state.platform1Pressure = m_current_state.getVW(plc_address::VW_PLATFORM1_PRESSURE) / 100.0;
    
    // 解析VW108: 升降平台2上升停止压力值
    m_current_state.platform2Pressure = m_current_state.getVW(plc_address::VW_PLATFORM2_PRESSURE) / 100.0;
    
    // 解析VW112: 平台倾斜角度
    m_current_state.tiltAngle = m_current_state.getVW(plc_address::VW_TILT_ANGLE) / 10.0; // 假设原始值需要除以10
    
    // 解析VW116: 平台位置信息
    m_current_state.platformPosition = m_current_state.getVW(plc_address::VW_POSITION);
}

/**
 * @brief 向PLC写入数据
 * @details 解析命令并写入对应的PLC地址
 * @param cmd 写入命令
 * @return 成功返回true，失败返回false
 */
bool PLCManager::write_plc_data(const std::string& cmd) {
    if (!m_is_connected || m_modbus_ctx == nullptr) {
        SPDLOG_ERROR("PLC未连接，无法执行写入操作");
        return false;
    }
    
    SPDLOG_DEBUG("准备向PLC写入命令: {}", cmd);
    
    // 解析命令格式：操作类型 参数
    std::istringstream iss(cmd);
    std::string operation;
    iss >> operation;
    
    int result = -1;
    
    // 根据PLC地址表处理不同操作
    if (operation == "SET_MODE") {
        std::string mode;
        iss >> mode;
        uint8_t value = (mode == "manual") ? 1 : 2; // 1=手动, 2=自动
        result = modbus_write_bit(m_modbus_ctx, plc_address::VB_OPERATION_MODE, value);
    }
    else if (operation == "EMERGENCY_STOP") {
        result = modbus_write_bit(m_modbus_ctx, plc_address::VB_EMERGENCY_STOP, 2); // 2=急停
    }
    else if (operation == "RESET") {
        result = modbus_write_bit(m_modbus_ctx, plc_address::VB_EMERGENCY_STOP, 1); // 1=复位
    }
    else if (operation == "OIL_PUMP") {
        std::string state;
        iss >> state;
        uint8_t value = (state == "start") ? 2 : 1; // 1=停止, 2=启动
        result = modbus_write_bit(m_modbus_ctx, plc_address::VB_OIL_PUMP, value);
    }
    else if (operation == "CYLINDER") {
        std::string action;
        iss >> action;
        
        uint8_t value = 0;
        if (action == "down_stop") {
            value = 1; // 下降停止
        }
        else if (action == "down_pressure") {
            value = 2; // 下降加压
        }
        else if (action == "up_stop") {
            value = 4; // 上升停止
        }
        else if (action == "up") {
            value = 8; // 上升
        }
        result = modbus_write_bit(m_modbus_ctx, plc_address::VB_CYLINDER_STATE, value);
    }
    else if (operation == "PLATFORM1") {
        std::string action;
        iss >> action;
        
        uint8_t value = 0;
        if (action == "up") {
            value = 1; // 上升
        }
        else if (action == "up_stop") {
            value = 2; // 上升停止
        }
        else if (action == "down") {
            value = 4; // 下降
        }
        else if (action == "down_stop") {
            value = 8; // 下降停止
        }
        result = modbus_write_bit(m_modbus_ctx, plc_address::VB_LIFT_PLATFORM1, value);
    }
    else if (operation == "PLATFORM2") {
        std::string action;
        iss >> action;
        
        uint8_t value = 0;
        if (action == "up") {
            value = 1; // 上升
        }
        else if (action == "up_stop") {
            value = 2; // 上升停止
        }
        else if (action == "down") {
            value = 4; // 下降
        }
        else if (action == "down_stop") {
            value = 8; // 下降停止
        }
        result = modbus_write_bit(m_modbus_ctx, plc_address::VB_LIFT_PLATFORM2, value);
    }
    else if (operation == "HEATER") {
        std::string state;
        iss >> state;
        uint8_t value = (state == "start") ? 2 : 1; // 1=停止, 2=启动
        result = modbus_write_bit(m_modbus_ctx, plc_address::VB_HEATER, value);
    }
    else if (operation == "COOLING") {
        std::string state;
        iss >> state;
        uint8_t value = (state == "start") ? 2 : 1; // 1=停止, 2=启动
        result = modbus_write_bit(m_modbus_ctx, plc_address::VB_AIR_COOLING, value);
    }
    else if (operation == "LEVELING") {
        std::string state;
        iss >> state;
        uint8_t value = (state == "start") ? 2 : 1; // 1=停止, 2=启动
        result = modbus_write_bit(m_modbus_ctx, plc_address::VB_LEVELING, value);
    }
    else {
        SPDLOG_ERROR("未知的PLC操作命令: {}", cmd);
        return false;
    }
    
    if (result == -1) {
        SPDLOG_ERROR("PLC写入操作失败: {}", modbus_strerror(errno));
        return false;
    }
    
    SPDLOG_INFO("PLC写入操作成功");
    
    // 读取最新状态
    return read_plc_data();
}

/**
 * @brief 执行高级业务操作
 * @details 将业务层面的操作转换为PLC层面的命令并执行
 * @param operation 操作指令
 */
void PLCManager::execute_operation(const std::string& operation) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 将业务操作转换为PLC命令
    bool success = false;
    
    if (operation == "自动模式") {
        success = write_plc_data("SET_MODE auto");
    }
    else if (operation == "手动模式") {
        success = write_plc_data("SET_MODE manual");
    }
    else if (operation == "急停") {
        success = write_plc_data("EMERGENCY_STOP");
    }
    else if (operation == "复位") {
        success = write_plc_data("RESET");
    }
    else if (operation == "油泵启动") {
        success = write_plc_data("OIL_PUMP start");
    }
    else if (operation == "油泵停止") {
        success = write_plc_data("OIL_PUMP stop");
    }
    else if (operation == "刚柔缸下降停止") {
        success = write_plc_data("CYLINDER down_stop");
    }
    else if (operation == "刚柔缸下降加压") {
        success = write_plc_data("CYLINDER down_pressure");
    }
    else if (operation == "刚柔缸上升停止") {
        success = write_plc_data("CYLINDER up_stop");
    }
    else if (operation == "刚柔缸上升") {
        success = write_plc_data("CYLINDER up");
    }
    else if (operation == "平台1上升") {
        success = write_plc_data("PLATFORM1 up");
    }
    else if (operation == "平台1上升停止") {
        success = write_plc_data("PLATFORM1 up_stop");
    }
    else if (operation == "平台1下降") {
        success = write_plc_data("PLATFORM1 down");
    }
    else if (operation == "平台1下降停止") {
        success = write_plc_data("PLATFORM1 down_stop");
    }
    else if (operation == "平台2上升") {
        success = write_plc_data("PLATFORM2 up");
    }
    else if (operation == "平台2上升停止") {
        success = write_plc_data("PLATFORM2 up_stop");
    }
    else if (operation == "平台2下降") {
        success = write_plc_data("PLATFORM2 down");
    }
    else if (operation == "平台2下降停止") {
        success = write_plc_data("PLATFORM2 down_stop");
    }
    else if (operation == "电加热启动") {
        success = write_plc_data("HEATER start");
    }
    else if (operation == "电加热停止") {
        success = write_plc_data("HEATER stop");
    }
    else if (operation == "风冷启动") {
        success = write_plc_data("COOLING start");
    }
    else if (operation == "风冷停止") {
        success = write_plc_data("COOLING stop");
    }
    else if (operation == "调平启动") {
        success = write_plc_data("LEVELING start");
    }
    else if (operation == "调平停止") {
        success = write_plc_data("LEVELING stop");
    }
    else {
        SPDLOG_WARN("未实现的PLC操作: {}", operation);
    }
    
    if (!success) {
        SPDLOG_ERROR("执行操作失败: {}", operation);
    }
}