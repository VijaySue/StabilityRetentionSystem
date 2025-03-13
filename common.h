// common.h
#pragma once
#include <cpprest/json.h>
#include <string>

namespace constants {
    const std::string VERSION = "1.0";        // 系统版本
    const int MAX_CONCURRENT = 100;           // 最大并发任务数
}

/**
 * @brief 设备状态数据结构体
 * @note 根据文档3.4章节PLC地址表定义字段
 */
struct DeviceState {
    // 操作模式 (VB1000)
    std::string operationMode;    // "手动"/"自动"

    // 急停状态 (VB1003)
    std::string emergencyStop;    // "复位"/"急停"

    // 刚柔配状态 (VB1003)
    std::string supportMode;      // "刚性支撑"/"柔性复位"

    // 升降平台状态
    std::string lift1State;       // VB1004
    std::string lift2State;       // VB1005

    // 传感器数据
    double lift1Pressure;         // VW104
    double lift2Pressure;         // VW106
    int tiltAngle;                // VW112
    int position;                 // VW116

    // 系统状态
    std::string alarmStatus;      // VB1008
};

// 状态转换工具函数
web::json::value device_state_to_json(const DeviceState& state);