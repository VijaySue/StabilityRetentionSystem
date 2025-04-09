/**
 * @file plc_manager.cpp
 * @brief PLC通信管理器实现
 * @details 处理与西门子S7 PLC通信，读写PLC数据并解析状态
 * @author VijaySue
 * @date 2024-3-29
 */
#include "../include/plc_manager.h"
#include "../include/config_manager.h"
#include <spdlog/spdlog.h>
#include <map>
#include <sstream>
#include <chrono>
#include <thread>
#include <snap7.h>

// 静态成员变量定义
TS7Client* PLCManager::m_client = nullptr;
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
    SPDLOG_INFO("开始尝试连接PLC设备，IP: {}, 端口: {}", get_plc_ip(), get_plc_port());
    
    // 释放之前的连接
    if (m_client != nullptr) {
        try {
            // 先尝试安全断开
            m_client->Disconnect();
        } catch (const std::exception& e) {
            SPDLOG_WARN("断开旧连接时出现异常: {}", e.what());
            // 异常不影响后续流程，继续释放资源
        }
        delete m_client;
        m_client = nullptr;
        m_is_connected = false;
    }

    // 连接重试次数和间隔
    const int MAX_RETRY = 3;  // 最大快速重试次数
    const int INITIAL_RETRY_DELAY_MS = 1000;  // 初始重试间隔1秒
    const float BACKOFF_FACTOR = 1.5f;  // 指数退避因子
    
    int retry_count = 0;
    
    // 先尝试快速连接几次
    while (retry_count < MAX_RETRY) {
        if (retry_count > 0) {
            // 前几次使用指数退避
            int delay_ms = static_cast<int>(INITIAL_RETRY_DELAY_MS * std::pow(BACKOFF_FACTOR, retry_count - 1));
            SPDLOG_INFO("第{}次重试连接PLC，等待{}毫秒...", retry_count, delay_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
        
        try {
            // 创建新的Snap7客户端
            m_client = new TS7Client();
            if (m_client == nullptr) {
                SPDLOG_ERROR("创建Snap7客户端失败");
                retry_count++;
                continue;
            }

            // 设置连接超时时间（默认是3秒）
            m_client->SetConnectionType(CONNTYPE_BASIC);
            
            SPDLOG_INFO("设置连接参数: IP={}", get_plc_ip());
            m_client->SetConnectionParams(get_plc_ip().c_str(), 0, 1);

            SPDLOG_INFO("开始连接到PLC...");
            int result = m_client->ConnectTo(get_plc_ip().c_str(), 0, 1);
            if (result != 0) {
                char error_text[256];
                Cli_ErrorText(result, error_text, sizeof(error_text));
                SPDLOG_ERROR("连接到PLC设备失败: 错误码 {}, 错误信息: {}", result, error_text);
                
                // 确保释放资源
                delete m_client;
                m_client = nullptr;
                m_is_connected = false;
                retry_count++;
                continue;
            }

            m_is_connected = true;
            SPDLOG_INFO("成功连接到西门子PLC设备，IP: {}", get_plc_ip());
            
            // 添加连接稳定化延迟，让PLC有时间准备好通信
            const int STABILIZATION_DELAY_MS = 500;
            SPDLOG_INFO("等待{}毫秒让PLC通信层稳定...", STABILIZATION_DELAY_MS);
            std::this_thread::sleep_for(std::chrono::milliseconds(STABILIZATION_DELAY_MS));
            
            return true;
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("连接PLC设备时发生异常: {}", e.what());
            if (m_client != nullptr) {
                delete m_client;
                m_client = nullptr;
            }
            m_is_connected = false;
            retry_count++;
            
            // 在重试之前添加短暂延迟，确保文件描述符有时间被操作系统释放
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    // 快速重试失败后，返回false让调用方知道连接失败
    // 这样AlarmMonitor可以上报连接失败
    m_is_connected = false;
    SPDLOG_ERROR("PLC连接失败，已达到最大重试次数({})", MAX_RETRY);
    return false;
}

/**
 * @brief 断开PLC连接
 */
void PLCManager::disconnect_plc() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_client != nullptr) {
        m_client->Disconnect();
        delete m_client;
        m_client = nullptr;
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
    
    // 检查是否已连接，如未连接则尝试重连
    if (!m_is_connected || m_client == nullptr) {
        SPDLOG_ERROR("PLC未连接，尝试重新连接...");
        if (!connect_plc()) {
            SPDLOG_ERROR("PLC连接失败");
            throw std::runtime_error("PLC连接失败");
        }
    }
    
    // 为避免文件描述符泄漏，使用局部变量跟踪是否需要重新连接
    bool need_reconnect = false;
    
    try {
        // 尝试从PLC读取数据
        if (!read_plc_data()) {
            // 读取失败，标记需要重新连接
            SPDLOG_ERROR("无法从PLC读取数据，尝试重新连接...");
            need_reconnect = true;
        } else {
            // 读取成功，解析数据
            parse_raw_values();
            return m_current_state;
        }
    }
    catch (const std::exception& e) {
        SPDLOG_ERROR("从PLC读取数据时发生异常: {}", e.what());
        need_reconnect = true;
    }
    
    // 如果需要重连，确保先释放当前连接
    if (need_reconnect) {
        // 确保明确断开旧连接并释放资源
        if (m_client != nullptr) {
            m_client->Disconnect();
            delete m_client;
            m_client = nullptr;
        }
        m_is_connected = false;
        
        // 尝试重新连接
        if (connect_plc()) {
            if (!read_plc_data()) {
                SPDLOG_ERROR("重连后仍无法从PLC读取数据");
                throw std::runtime_error("无法从PLC读取数据");
            }
            parse_raw_values();
            return m_current_state;
        } else {
            SPDLOG_ERROR("PLC重连失败");
            throw std::runtime_error("PLC重连失败");
        }
    }
    
    return m_current_state;
}

/**
* @brief 调整字节流顺序
* @details plc存储是大端序，而主机是小端序，需调整顺序解析正确数值
* @return PLC中存储的正确数值
*/
float PLCManager::bytesSwap(const byte *bytes) {
    byte reordered[4];
    // 调整字节序：PLC的大端序转为主机序
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    reordered[0] = bytes[3];
    reordered[1] = bytes[2];
    reordered[2] = bytes[1];
    reordered[3] = bytes[0];
#else
    memcpy(reordered, bytes, 4);
#endif
    float value;
    memcpy(&value, reordered, 4);
    return value;
}

/**
 * @brief 从PLC读取原始数据
 * @details 使用Snap7读取PLC的数据区域
 * @return 成功返回true，失败返回false
 */
bool PLCManager::read_plc_data() {
    // 检查是否已连接
    if (!m_is_connected || m_client == nullptr) {
        SPDLOG_ERROR("PLC未连接，无法读取数据");
        return false;
    }
    
    // 创建缓冲区用于读取数据
    byte buffer[16] = {0};  // 数据缓冲区
    float value = 0;

    int result;
    
    // 读取VB1000控制字节（包含多个位状态）
    result = m_client->ReadArea(S7AreaDB, 1, 1000, 1, S7WLByte, &buffer[0]);
    if (result != 0) {
        // 获取错误描述
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        
        SPDLOG_ERROR("读取VB1000控制字节失败: 错误码 {}, 错误信息: {}", result, error_text);
        
        // 如果是连接错误，标记连接状态
        if (result == 32) { // 错误码32通常表示连接已断开
            SPDLOG_ERROR("检测到PLC连接已断开");
            m_is_connected = false; // 更新连接状态
        }
        
        return false;
    }
    m_current_state.setVB(plc_address::VB_CONTROL_BYTE, buffer[0]);
    
    // 读取VB1001: 刚柔缸状态
    result = m_client->ReadArea(S7AreaDB, 1, 1001, 1, S7WLByte, &buffer[0]);
    if (result != 0) {
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VB1001刚柔缸状态失败: 错误码 {}, 错误信息: {}", result, error_text);
        
        // 检查是否是连接错误
        if (result == 32) {
            m_is_connected = false;
        }
        return false;
    }
    m_current_state.setVB(plc_address::VB_CYLINDER_STATE, buffer[0]);
    
    // 读取VB1002: 升降平台1状态
    result = m_client->ReadArea(S7AreaDB, 1, 1002, 1, S7WLByte, &buffer[0]);
    if (result != 0) {
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VB1002升降平台1状态失败: 错误码 {}, 错误信息: {}", result, error_text);
        
        // 检查是否是连接错误
        if (result == 32) {
            m_is_connected = false;
        }
        return false;
    }
    m_current_state.setVB(plc_address::VB_LIFT_PLATFORM1, buffer[0]);
    
    // 读取VB1003: 升降平台2状态
    result = m_client->ReadArea(S7AreaDB, 1, 1003, 1, S7WLByte, &buffer[0]);
    if (result != 0) {
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VB1003升降平台2状态失败: 错误码 {}, 错误信息: {}", result, error_text);
        
        // 检查是否是连接错误
        if (result == 32) {
            m_is_connected = false;
        }
        return false;
    }
    m_current_state.setVB(plc_address::VB_LIFT_PLATFORM2, buffer[0]);
    
    // 读取VB1004: 报警信号
    result = m_client->ReadArea(S7AreaDB, 1, 1004, 1, S7WLByte, &buffer[0]);
    if (result != 0) {
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VB1004报警信号失败: 错误码 {}, 错误信息: {}", result, error_text);
        
        // 检查是否是连接错误
        if (result == 32) {
            m_is_connected = false;
        }
        return false;
    }
    m_current_state.setVB(plc_address::VB_ALARM, buffer[0]);
    
    // 读取VD1010: 刚柔缸下降停止压力值
    result = m_client->ReadArea(S7AreaDB, 1, 1010, 4, S7WLReal, &buffer);
    value = bytesSwap(buffer);
    if (result != 0) {
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VD1010刚柔缸下降停止压力值失败: 错误码 {}, 错误信息: {}", result, error_text);
        
        // 检查是否是连接错误
        if (result == 32) {
            m_is_connected = false;
        }
        return false;
    }
    m_current_state.setVD(plc_address::VD_CYLINDER_PRESSURE, value);
    
    // 读取VD1014: 升降平台上升停止压力值
    result = m_client->ReadArea(S7AreaDB, 1, 1014, 4, S7WLReal, &value);
    value = bytesSwap(buffer);
    if (result != 0) {
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VD1014升降平台上升停止压力值失败: 错误码 {}, 错误信息: {}", result, error_text);
        
        // 检查是否是连接错误
        if (result == 32) {
            m_is_connected = false;
        }
        return false;
    }
    m_current_state.setVD(plc_address::VD_LIFT_PRESSURE, value);
    
    // 读取VD1018: 平台1倾斜角度
    result = m_client->ReadArea(S7AreaDB, 1, 1018, 4, S7WLReal, &buffer);
    value = bytesSwap(buffer);
    if (result != 0) {
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VD1018平台1倾斜角度失败: 错误码 {}, 错误信息: {}", result, error_text);
        
        // 检查是否是连接错误
        if (result == 32) {
            m_is_connected = false;
        }
        return false;
    }
    m_current_state.setVD(plc_address::VD_PLATFORM1_TILT, value);
    
    // 读取VD1022: 平台2倾斜角度
    result = m_client->ReadArea(S7AreaDB, 1, 1022, 4, S7WLReal, &buffer);
    value = bytesSwap(buffer);
    if (result != 0) {
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VD1022平台2倾斜角度失败: 错误码 {}, 错误信息: {}", result, error_text);
        
        // 检查是否是连接错误
        if (result == 32) {
            m_is_connected = false;
        }
        return false;
    }
    m_current_state.setVD(plc_address::VD_PLATFORM2_TILT, value);
    
    // 读取VD1026: 平台1位置信息
    result = m_client->ReadArea(S7AreaDB, 1, 1026, 4, S7WLReal, &buffer);
    value = bytesSwap(buffer);
    if (result != 0) {
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VD1026平台1位置信息失败: 错误码 {}, 错误信息: {}", result, error_text);
        
        // 检查是否是连接错误
        if (result == 32) {
            m_is_connected = false;
        }
        return false;
    }
    m_current_state.setVD(plc_address::VD_PLATFORM1_POS, value);
    
    // 读取VD1030: 平台2位置信息
    result = m_client->ReadArea(S7AreaDB, 1, 1030, 4, S7WLReal, &buffer);
    value = bytesSwap(buffer);
    if (result != 0) {
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VD1030平台2位置信息失败: 错误码 {}, 错误信息: {}", result, error_text);
        
        // 检查是否是连接错误
        if (result == 32) {
            m_is_connected = false;
        }
        return false;
    }
    m_current_state.setVD(plc_address::VD_PLATFORM2_POS, value);
    
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
    
    // 解析VD1010: 刚柔缸下降停止压力值
    m_current_state.cylinderPressure = m_current_state.getVD(plc_address::VD_CYLINDER_PRESSURE);
    
    // 解析VD1014: 升降平台上升停止压力值
    m_current_state.liftPressure = m_current_state.getVD(plc_address::VD_LIFT_PRESSURE);
    
    // 解析VD1018: 平台1倾斜角度
    m_current_state.platform1TiltAngle = m_current_state.getVD(plc_address::VD_PLATFORM1_TILT);
    
    // 解析VD1022: 平台2倾斜角度
    m_current_state.platform2TiltAngle = m_current_state.getVD(plc_address::VD_PLATFORM2_TILT);
    
    // 解析VD1026: 平台1位置信息
    m_current_state.platform1Position = m_current_state.getVD(plc_address::VD_PLATFORM1_POS);
    
    // 解析VD1030: 平台2位置信息
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
    
    // 检查是否已连接，如未连接则尝试重连
    if (!m_is_connected || m_client == nullptr) {
        SPDLOG_ERROR("PLC未连接，尝试重新连接...");
        if (!connect_plc()) {
            SPDLOG_ERROR("PLC连接失败，无法执行操作: {}", operation);
            return false;
        }
    }
    
    int result = -1;
    // 创建一个字节的缓冲区，用于写入值1
    byte buffer_on[1] = {0x01};  // 0x01 表示值为1
    int address = -1;  // M地址的位偏移量
    std::string address_desc; // M地址描述，用于日志
    
    // 仅保留M地址操作
    if (operation == "刚性支撑") {  // JSON请求中state="刚性支撑"
        // 对应M22.1 - Snap7以0开始的字节偏移和位偏移
        address = 22*8 + 1;
        address_desc = "M22.1";
        SPDLOG_DEBUG("执行刚性支撑命令，写入M22.1=1");
    }
    else if (operation == "柔性复位") {  // JSON请求中state="柔性复位"
        // 对应M22.2
        address = 22*8 + 2;
        address_desc = "M22.2";
        SPDLOG_DEBUG("执行柔性复位命令，写入M22.2=1");
    }
    else if (operation == "平台1上升" || operation == "平台1升高") {  // JSON请求中state="升高"，platformNum=1
        // 对应M22.3
        address = 22*8 + 3;
        address_desc = "M22.3";
        SPDLOG_DEBUG("执行平台1上升命令，写入M22.3=1");
    }
    else if (operation == "平台1下降" || operation == "平台1复位") {  // JSON请求中state="复位"，platformNum=1
        // 对应M22.4
        address = 22*8 + 4;
        address_desc = "M22.4";
        SPDLOG_DEBUG("执行平台1复位命令，写入M22.4=1");
    }
    else if (operation == "平台2上升" || operation == "平台2升高") {  // JSON请求中state="升高"，platformNum=2
        // 对应M22.5
        address = 22*8 + 5;
        address_desc = "M22.5";
        SPDLOG_DEBUG("执行平台2上升命令，写入M22.5=1");
    }
    else if (operation == "平台2下降" || operation == "平台2复位") {  // JSON请求中state="复位"，platformNum=2
        // 对应M22.6
        address = 22*8 + 6;
        address_desc = "M22.6";
        SPDLOG_DEBUG("执行平台2复位命令，写入M22.6=1");
    }
    else if (operation == "平台1调平" || operation == "1号平台调平") {  // JSON请求中state="调平"，platformNum=1
        // 对应M22.7
        address = 22*8 + 7;
        address_desc = "M22.7";
        SPDLOG_DEBUG("执行平台1调平命令，写入M22.7=1");
    }
    else if (operation == "平台1调平复位" || operation == "1号平台调平复位") {  // JSON请求中state="调平复位"，platformNum=1
        // 对应M23.0
        address = 23*8 + 0;
        address_desc = "M23.0";
        SPDLOG_DEBUG("执行平台1调平复位命令，写入M23.0=1");
    }
    else if (operation == "平台2调平" || operation == "2号平台调平") {  // JSON请求中state="调平"，platformNum=2
        // 对应M23.1
        address = 23*8 + 1;
        address_desc = "M23.1";
        SPDLOG_DEBUG("执行平台2调平命令，写入M23.1=1");
    }
    else if (operation == "平台2调平复位" || operation == "2号平台调平复位") {  // JSON请求中state="调平复位"，platformNum=2
        // 对应M23.2
        address = 23*8 + 2;
        address_desc = "M23.2";
        SPDLOG_DEBUG("执行平台2调平复位命令，写入M23.2=1");
    }
    else {
        SPDLOG_WARN("未实现的PLC操作: {}", operation);
        return false;
    }
    
    // 如果找到有效地址，写入位=1
    if (address >= 0) {
        result = m_client->WriteArea(S7AreaMK, 0, address, 1, S7WLBit, buffer_on);
    }
    
    if (result != 0) {
        // 获取错误描述
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        
        SPDLOG_ERROR("执行操作失败: {} (错误码: {}, 错误信息: {})", operation, result, error_text);
        
        // 如果错误表明连接已断开，尝试重新连接
        if (result == 32) { // 错误码32通常表示连接已断开
            SPDLOG_ERROR("检测到PLC连接断开，尝试重新连接...");
            m_is_connected = false; // 标记为未连接状态
            if (connect_plc()) {
                SPDLOG_INFO("PLC重新连接成功，重新尝试执行操作");
                // 重新尝试执行操作
                return execute_operation(operation);
            }
            SPDLOG_ERROR("PLC重连后仍然无法执行操作");
        }
        
        return false;  // 操作失败，返回false
    } else {
        SPDLOG_INFO("成功执行操作: {}, 1秒后自动复位", operation);
        
        // 创建一个线程，1秒后复位该位
        std::thread reset_thread([this, address, address_desc]() {
            // 等待1秒
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // 创建缓冲区，用于写入值0
            byte buffer_off[1] = {0x00}; // 0x00 表示值为0
            
            // 获取锁
            std::lock_guard<std::mutex> reset_lock(m_mutex);
            
            // 检查是否仍然连接
            if (m_is_connected && m_client != nullptr) {
                // 复位位
                int reset_result = m_client->WriteArea(S7AreaMK, 0, address, 1, S7WLBit, buffer_off);
                if (reset_result == 0) {
                    SPDLOG_DEBUG("已复位{}=0", address_desc);
                } else {
                    char reset_error_text[256];
                    Cli_ErrorText(reset_result, reset_error_text, sizeof(reset_error_text));
                    SPDLOG_ERROR("复位{}=0失败: 错误码 {}, 错误信息: {}", 
                        address_desc, reset_result, reset_error_text);
                }
            } else {
                SPDLOG_WARN("PLC已断开，无法复位{}=0", address_desc);
            }
        });
        
        // 分离线程让它在后台运行
        reset_thread.detach();
        
        return true;   // 操作成功，返回true
    }
}

/**
 * @brief 从PLC读取报警信号
 * @details 读取三个报警信号地址(VB1004油温、VB1005液位、VB1006滤芯)的数据，用于报警监控
 * @return 包含三个报警信号值的结构体，如果读取失败对应字段返回255
 */
AlarmSignals PLCManager::read_alarm_signal() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 创建并初始化返回结构体
    AlarmSignals signals = {255, 255, 255}; // 默认全部为255表示连接故障
    
    // 检查是否已连接
    if (!m_is_connected || m_client == nullptr) {
        SPDLOG_ERROR("PLC未连接，无法读取报警信号");
        m_is_connected = false;  // 确保标记为未连接状态
        return signals;  // 直接返回全部故障
    }
    
    // 添加读取之前的稳定性检查，确保连接真正稳定
    try {
        if (!m_client->Connected()) {
            SPDLOG_ERROR("PLC连接状态检查失败，将重置连接状态");
            m_is_connected = false;
            return signals;
        }
    }
    catch (const std::exception& e) {
        SPDLOG_ERROR("PLC连接检查异常: {}", e.what());
        m_is_connected = false;
        return signals;
    }
    
    // 创建三个缓冲区用于读取数据
    byte buffer[1] = {0};
    int result;
    
    // 读取VB1004报警信号（油温）- 使用常量
    result = m_client->ReadArea(S7AreaDB, 1, plc_address::VB_ALARM_OIL_TEMP, 1, S7WLByte, buffer);
    if (result == 0) {
        signals.oil_temp = buffer[0];
    } else {
        // 获取错误描述
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VB{}油温报警信号失败: 错误码 {}, 错误信息: {}", 
            plc_address::VB_ALARM_OIL_TEMP, result, error_text);
        
        // 如果是连接错误，标记连接状态
        if (result == 32) { // 错误码32通常表示连接已断开
            SPDLOG_ERROR("检测到PLC连接已断开");
            m_is_connected = false;
            return signals; // 连接已断开，直接返回
        }
    }
    
    // 读取VB1005报警信号（液位）- 使用常量
    result = m_client->ReadArea(S7AreaDB, 1, plc_address::VB_ALARM_LIQUID_LEVEL, 1, S7WLByte, buffer);
    if (result == 0) {
        signals.liquid_level = buffer[0];
    } else {
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VB{}液位报警信号失败: 错误码 {}, 错误信息: {}", 
            plc_address::VB_ALARM_LIQUID_LEVEL, result, error_text);
    }
    
    // 读取VB1006报警信号（滤芯）- 使用常量
    result = m_client->ReadArea(S7AreaDB, 1, plc_address::VB_ALARM_FILTER, 1, S7WLByte, buffer);
    if (result == 0) {
        signals.filter = buffer[0];
    } else {
        char error_text[256];
        Cli_ErrorText(result, error_text, sizeof(error_text));
        SPDLOG_ERROR("读取VB{}滤芯报警信号失败: 错误码 {}, 错误信息: {}", 
            plc_address::VB_ALARM_FILTER, result, error_text);
    }
    
    return signals;
}