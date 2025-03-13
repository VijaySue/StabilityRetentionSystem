// common.cpp
#include "common.h"
#include <cpprest/json.h>

web::json::value device_state_to_json(const DeviceState& state) {
    web::json::value root;

    // 基本状态信息
    root["operation_mode"] = web::json::value::string(state.operationMode);
    root["emergency_stop"] = web::json::value::string(state.emergencyStop);

    // 升降平台状态
    root["lift1_state"] = web::json::value::string(state.lift1State);
    root["lift2_state"] = web::json::value::string(state.lift2State);

    // 传感器数值
    root["lift1_pressure"] = state.lift1Pressure;
    root["lift2_pressure"] = state.lift2Pressure;
    root["tilt_angle"] = state.tiltAngle;
    root["position"] = state.position;

    // 系统状态
    root["alarm_status"] = web::json::value::string(state.alarmStatus);
    root["api_version"] = web::json::value::string(constants::VERSION);

    // 支撑状态
    root["support_mode"] = web::json::value::string(state.supportMode);

    return root;
}