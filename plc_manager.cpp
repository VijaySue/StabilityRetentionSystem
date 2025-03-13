// plc_manager.cpp
#include "plc_manager.h"
#include <spdlog/spdlog.h>
#include <random>

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
    // 模拟读取PLC寄存器（根据文档3.4地址表）

    // 操作模式 (VB1000)
    m_current_state.operationMode = (rand() % 2) ? "手动" : "自动";

    // 急停状态 (VB1003)
    m_current_state.emergencyStop = (rand() % 2) ? "复位" : "急停";

    // 升降平台1状态 (VB1004)
    static const std::vector<std::string> liftStates = {
        "上升", "上升停止", "下降", "下降停止"
    };
    m_current_state.lift1State = liftStates[rand() % 4];
    m_current_state.lift2State = liftStates[rand() % 4];

    // 压力值 (VW104/VW106)
    m_current_state.lift1Pressure = 100.0 + (rand() % 1000) / 10.0;
    m_current_state.lift2Pressure = 100.0 + (rand() % 1000) / 10.0;

    // 倾斜角度 (VW112)
    m_current_state.tiltAngle = rand() % 360;

    // 位置信息 (VW116)
    m_current_state.position = rand() % 1000;

    // 报警状态 (VB1008)
    static const std::vector<std::string> alarmStates = {
        "正常", "油温低", "油位高", "滤芯堵塞"
    };
    m_current_state.alarmStatus = alarmStates[rand() % 4];

    // 添加更多状态模拟
    m_current_state.operationMode = (rand() % 2) ? "手动模式" : "自动模式";
    m_current_state.emergencyStop = (rand() % 2) ? "正常" : "急停中";

    // 添加更多传感器数据
    m_current_state.tiltAngle = rand() % 10;  // 模拟0-9度倾斜
    m_current_state.position = 500 + rand() % 1000;  // 模拟位置数据
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