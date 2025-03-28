/**
 * @file common.cpp
 * @brief 公共工具函数实现
 * @details 实现设备状态到JSON的转换等公共功能
 * @author VijaySue
 * @date 2024-3-11
 * @platform Windows开发，Linux远程调试
 */
#include "../include/common.h"
#include <nlohmann/json.hpp>
#include <chrono>

using json = nlohmann::json;

/**
 * @brief 将设备状态转换为JSON字符串
 * @details 将DeviceState结构中的所有状态信息转换为标准的JSON格式
 *          按照API文档要求实现实时状态获取接口响应格式
 * @param state 设备状态对象
 * @return 包含设备状态的JSON字符串
 */
std::string device_state_to_json(const DeviceState& state) {
    // 构建JSON响应
    json response;
    
    // 基本响应信息
    response["msg"] = constants::MSG_SUCCESS;
    response["code"] = 200;
    
    // 完整状态信息
    response["operationMode"] = state.operationMode;     // 操作模式
    response["emergencyStop"] = state.emergencyStop;     // 急停状态
    response["oilPumpStatus"] = state.oilPumpStatus;     // 油泵状态
    response["cylinderState"] = state.cylinderState;     // 刚柔缸状态
    response["platform1State"] = state.platform1State;   // 升降平台1状态
    response["platform2State"] = state.platform2State;   // 升降平台2状态
    response["heaterStatus"] = state.heaterStatus;       // 电加热状态
    response["coolingStatus"] = state.coolingStatus;     // 风冷状态
    response["leveling1Status"] = state.leveling1Status; // 1#电动缸调平
    response["leveling2Status"] = state.leveling2Status; // 2#电动缸调平
    
    // 数值指标
    response["cylinderPressure"] = state.cylinderPressure;         // 刚柔缸压力值
    response["liftPressure"] = state.liftPressure;                 // 升降平台上升停止压力值
    response["platform1TiltAngle"] = state.platform1TiltAngle;     // 平台1倾斜角度
    response["platform2TiltAngle"] = state.platform2TiltAngle;     // 平台2倾斜角度
    response["platform1Position"] = state.platform1Position;       // 平台1位置信息
    response["platform2Position"] = state.platform2Position;       // 平台2位置信息
    
    // 设置时间戳
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    response["timestamp"] = timestamp;
    
    return response.dump();
}