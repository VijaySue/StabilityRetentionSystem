/**
 * @file common.cpp
 * @brief 公共工具函数实现
 * @details 实现设备状态到JSON的转换等公共功能
 * @author VijaySue
 * @version 2.0
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
 *          支持原始数据获取，便于调试和问题排查
 * @param state 设备状态对象
 * @return 包含设备状态的JSON字符串
 */
std::string device_state_to_json(const DeviceState& state) {
    // 构建JSON响应
    json response;
    
    // 基本响应信息
    response["msg"] = constants::MSG_SUCCESS;
    
    // 主要状态信息 - 保留接口文档中的liftState和liftPressure向后兼容
    response["liftState"] = state.platform1State;  // 文档要求：对应图中升降平台状态
    response["liftPressure"] = state.platform1Pressure; // 文档要求：对应图中升降平台压力值
    
    // 完整状态信息
    response["operationMode"] = state.operationMode;     // 自动按钮状态
    response["emergencyStop"] = state.emergencyStop;     // 急停按钮状态
    response["oilPumpStatus"] = state.oilPumpStatus;     // 油泵状态
    response["cylinderState"] = state.cylinderState;     // 刚柔缸状态
    response["platform1State"] = state.platform1State;   // 升降平台1状态
    response["platform2State"] = state.platform2State;   // 升降平台2状态
    response["heaterStatus"] = state.heaterStatus;       // 电加热状态
    response["coolingStatus"] = state.coolingStatus;     // 风冷状态
    response["alarmStatus"] = state.alarmStatus;         // 报警状态
    response["levelingStatus"] = state.levelingStatus;   // 电动缸调平状态
    
    // 数值指标
    response["cylinderPressure"] = state.cylinderPressure;       // 刚柔缸压力值
    response["platform1Pressure"] = state.platform1Pressure;     // 升降平台1压力值
    response["platform2Pressure"] = state.platform2Pressure;     // 升降平台2压力值
    response["tiltAngle"] = state.tiltAngle;                     // 平台倾斜角度
    response["platformPosition"] = state.platformPosition;       // 平台位置信息
    
    // 添加原始数据（用于调试）
    json raw_data;
    
    // 仅添加有实际使用的地址区域
    const uint16_t used_vb_addresses[] = {
        plc_address::VB_OPERATION_MODE,
        plc_address::VB_EMERGENCY_STOP,
        plc_address::VB_OIL_PUMP,
        plc_address::VB_CYLINDER_STATE,
        plc_address::VB_LIFT_PLATFORM1,
        plc_address::VB_LIFT_PLATFORM2,
        plc_address::VB_HEATER,
        plc_address::VB_AIR_COOLING,
        plc_address::VB_ALARM,
        plc_address::VB_LEVELING
    };
    
    const uint16_t used_vw_addresses[] = {
        plc_address::VW_CYLINDER_PRESSURE,
        plc_address::VW_PLATFORM1_PRESSURE,
        plc_address::VW_PLATFORM2_PRESSURE,
        plc_address::VW_TILT_ANGLE,
        plc_address::VW_POSITION
    };
    
    // 添加VB原始值
    for (const auto& addr : used_vb_addresses) {
        raw_data["vb_" + std::to_string(addr)] = static_cast<int>(state.getVB(addr));
    }
    
    // 添加VW原始值
    for (const auto& addr : used_vw_addresses) {
        raw_data["vw_" + std::to_string(addr)] = state.getVW(addr);
    }
    
    response["raw_data"] = raw_data;
    
    // 添加PLC连接状态（可选）
    // 此处假设我们已经有了PLCManager的实例
    // response["connectionStatus"] = PLCManager::instance().is_connected() ? "connected" : "disconnected";
    
    // 设置时间戳
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    response["timestamp"] = timestamp;
    
    return response.dump();
}