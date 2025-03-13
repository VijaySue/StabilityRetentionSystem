// plc_manager.cpp
#include "plc_manager.h"
#include <spdlog/spdlog.h>
#include <map>
#include <random>
#include <boost/algorithm/string/join.hpp>

PLCManager& PLCManager::instance() {
    static PLCManager instance;
    return instance;
}

PLCManager::PLCManager() {
    // 初始化模拟数据生成器
    std::srand(std::time(nullptr));
    SPDLOG_INFO("PLC管理器初始化完成");
}

DeviceState PLCManager::get_current_state() {
    std::lock_guard<std::mutex> lock(m_mutex);
    simulate_plc_read();  // 模拟读取PLC数据
    return m_current_state;
}

void PLCManager::simulate_plc_read() {
    /* 根据EXCEL1 Sheet1 PLC地址表精确模拟数据 */

    // VB1000: 操作模式（1字节）
    m_current_state.operationMode = (rand() % 2) ? "手动" : "自动";

    // VB1001: 急停状态（1字节）
    m_current_state.emergencyStop = (rand() % 2) ? "复位" : "急停";

    // VB1003: 刚柔缸状态（1字节）
    static const std::map<int, std::string> supportStates = {
        {1, "下降停止"}, {2, "下降加压"},
        {4, "上升停止"}, {8, "上升停止"} // 根据EXCEL备注修正
    };
    m_current_state.supportMode = supportStates.at(1 << (rand() % 4));

    // VB1004/VB1005: 升降平台状态（1字节）
    static const std::map<int, std::string> liftStates = {
        {1, "上升"}, {2, "上升停止"},
        {4, "下降"}, {8, "下降停止"}
    };
    m_current_state.lift1State = liftStates.at(1 << (rand() % 4));
    m_current_state.lift2State = liftStates.at(1 << (rand() % 4));

    // VW104/VW106: 压力值（2字节整型）
    m_current_state.lift1Pressure = rand() % 65535;  // 模拟16位无符号整型
    m_current_state.lift2Pressure = rand() % 65535;

    // VW112: 倾斜角度（2字节整型）
    m_current_state.tiltAngle = rand() % 360;

    // VW116: 位置信息（2字节整型）
    m_current_state.position = rand() % 65535;

    // VB1008: 报警信号（位域处理）
    std::vector<std::string> activeAlarms;
    uint8_t alarmByte = rand() % 256;
    if (alarmByte & 0x01) activeAlarms.push_back("油温低");
    if (alarmByte & 0x02) activeAlarms.push_back("油温高");
    if (alarmByte & 0x04) activeAlarms.push_back("液位低");
    if (alarmByte & 0x08) activeAlarms.push_back("液位高");
    if (alarmByte & 0x10) activeAlarms.push_back("滤芯堵塞");
    m_current_state.alarmStatus =
        activeAlarms.empty() ? "正常" : boost::algorithm::join(activeAlarms, ",");
}

void PLCManager::simulate_plc_write(const std::string& cmd) {
    SPDLOG_DEBUG("模拟PLC写入命令: {}", cmd);

    // 解析命令格式：操作类型 参数
    std::istringstream iss(cmd);
    std::string operation;
    iss >> operation;

    // 根据文档3.4的PLC地址表处理不同操作
    if (operation == "SET_MODE") {
        // 设置操作模式 VB1000
        std::string mode;
        iss >> mode;
        if (mode == "manual") {
            m_current_state.operationMode = "手动";
            SPDLOG_INFO("PLC设置操作模式: 手动");
        }
        else if (mode == "auto") {
            m_current_state.operationMode = "自动";
            SPDLOG_INFO("PLC设置操作模式: 自动");
        }
    }
    else if (operation == "EMERGENCY_STOP") {
        // 急停按钮 VB1003
        m_current_state.emergencyStop = "急停";
        SPDLOG_WARN("PLC触发急停操作");
    }
    else if (operation == "SET_SUPPORT") {
        std::string type;
        iss >> type;
        if (type == "rigid") {
            m_current_state.supportMode = "刚性支撑";
            SPDLOG_INFO("刚性支撑已激活");
        }
        else if (type == "flexible") {
            m_current_state.supportMode = "柔性复位";
            SPDLOG_INFO("柔性复位已激活");
        }
    }
    else if (operation == "MOVE_PLATFORM") {
        // 平台控制 格式：MOVE_PLATFORM [平台号] [动作]
        int platformNum;
        std::string action;
        iss >> platformNum >> action;

        // 根据文档3.4的VB1004/VB1005地址
        if (platformNum == 1) {
            if (action == "up") {
                m_current_state.lift1State = "上升";
                SPDLOG_INFO("平台1开始上升");
            }
            else if (action == "stop") {
                m_current_state.lift1State = "上升停止";
                SPDLOG_INFO("平台1停止");
            }
        }
        else if (platformNum == 2) {
            if (action == "up") {
                m_current_state.lift2State = "上升";
                SPDLOG_INFO("平台2开始上升");
            }
            else if (action == "stop") {
                m_current_state.lift2State = "上升停止";
                SPDLOG_INFO("平台2停止");
            }
        }
    }
    else if (operation == "RESET") {
        // 复位操作 VB1003
        m_current_state.emergencyStop = "复位";
        SPDLOG_INFO("PLC执行系统复位");
    }
    else {
        SPDLOG_ERROR("未知的PLC操作命令: {}", cmd);
    }

    // 模拟写入延迟
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void PLCManager::execute_operation(const std::string& operation) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 将业务操作转换为PLC命令
    if (operation == "刚性支撑") {
        simulate_plc_write("SET_SUPPORT rigid");
    }
    else if (operation == "柔性复位") {
        simulate_plc_write("SET_SUPPORT flexible");
    }
    else if (operation.find("升高") != std::string::npos) {
        int platformNum = 1; // 从参数解析平台号
        simulate_plc_write("MOVE_PLATFORM " + std::to_string(platformNum) + " up");
    }
    else if (operation.find("复位") != std::string::npos) {
        simulate_plc_write("RESET");
    }
    else {
        SPDLOG_WARN("未实现的PLC操作: {}", operation);
    }
}