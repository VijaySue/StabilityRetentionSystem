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
 *          支持原始数据获取，便于调试和问题排查
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
    
    // 添加原始数据（用于调试）
    json raw_data;
    
    // 添加VB原始值 - 控制字节的位值
    raw_data["operation_mode_bit"] = state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_OPERATION_MODE);
    raw_data["emergency_stop_bit"] = state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_EMERGENCY_STOP);
    raw_data["oil_pump_bit"] = state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_OIL_PUMP);
    raw_data["heater_bit"] = state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_HEATER);
    raw_data["cooling_bit"] = state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_AIR_COOLING);
    raw_data["leveling1_bit"] = state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_LEVELING1);
    raw_data["leveling2_bit"] = state.isBitSet(plc_address::VB_CONTROL_BYTE, plc_address::BIT_LEVELING2);
    
    // 添加状态字节
    raw_data["cylinder_state"] = static_cast<int>(state.getVB(plc_address::VB_CYLINDER_STATE));
    raw_data["platform1_state"] = static_cast<int>(state.getVB(plc_address::VB_LIFT_PLATFORM1));
    raw_data["platform2_state"] = static_cast<int>(state.getVB(plc_address::VB_LIFT_PLATFORM2));
    raw_data["alarm_state"] = static_cast<int>(state.getVB(plc_address::VB_ALARM));
    
    // 添加VD浮点值
    raw_data["cylinder_pressure"] = state.getVD(plc_address::VD_CYLINDER_PRESSURE);
    raw_data["lift_pressure"] = state.getVD(plc_address::VD_LIFT_PRESSURE);
    raw_data["platform1_tilt"] = state.getVD(plc_address::VD_PLATFORM1_TILT);
    raw_data["platform2_tilt"] = state.getVD(plc_address::VD_PLATFORM2_TILT);
    raw_data["platform1_pos"] = state.getVD(plc_address::VD_PLATFORM1_POS);
    raw_data["platform2_pos"] = state.getVD(plc_address::VD_PLATFORM2_POS);
    
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