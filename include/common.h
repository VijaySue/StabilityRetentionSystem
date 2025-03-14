/**
 * @file common.h
 * @brief 公共定义和工具函数
 * @details 包含整个系统中共用的常量、类型定义、PLC地址映射和数据结构
 * @author VijaySue
 * @version 2.0
 * @date 2024-3-11
 * @platform Windows开发，Linux远程调试
 */
#pragma once
#include <string>
#include <cstdint>  // 使用标准类型定义

/**
 * @namespace constants
 * @brief 系统常量定义
 */
namespace constants {
    const std::string VERSION = "1.0";        // 系统版本
    const int MAX_CONCURRENT = 100;           // 最大并发任务数
    
    // PLC通信常量
    const int PLC_RETRY_COUNT = 3;            // PLC连接重试次数
    const int PLC_RETRY_INTERVAL_MS = 1000;   // PLC连接重试间隔(毫秒)
    
    // API响应常量
    const std::string MSG_SUCCESS = "success"; // 成功响应消息
    const std::string MSG_ERROR = "error";     // 错误响应消息
}

/**
 * @namespace plc_address
 * @brief PLC地址定义
 * @details 根据西门子S7系列PLC地址映射表定义，用于Modbus TCP通信
 * @note VB: 字节型(8位, uint8_t) VW: 字型(16位, int16_t)
 */
namespace plc_address {
    // VB 地址（字节类型，8位）
    const uint16_t VB_OPERATION_MODE = 1000;  // 自动按钮（操作）: 1=手动, 2=自动
    const uint16_t VB_EMERGENCY_STOP = 1001;  // 急停按钮: 1=复位, 2=急停
    const uint16_t VB_OIL_PUMP = 1002;        // 油泵状态: 1=停止, 2=启动
    const uint16_t VB_CYLINDER_STATE = 1003;  // 刚柔缸状态: 1=下降停止, 2=下降加压, 4=上升停止, 8=上升停止
    const uint16_t VB_LIFT_PLATFORM1 = 1004;  // 升降平台1状态: 1=上升, 2=上升停止, 4=下降, 8=下降停止
    const uint16_t VB_LIFT_PLATFORM2 = 1005;  // 升降平台2状态: 1=上升, 2=上升停止, 4=下降, 8=下降停止
    const uint16_t VB_HEATER = 1006;          // 电加热状态: 1=停止, 2=启动
    const uint16_t VB_AIR_COOLING = 1007;     // 风冷状态: 1=停止, 2=启动
    const uint16_t VB_ALARM = 1008;           // 报警信号: 0=油温低, 1=油温高, 2=液位低, 4=液位高, 8=滤芯堵
    const uint16_t VB_LEVELING = 1009;        // 电动缸调平: 1=停止, 2=启动
    
    // VW 地址（字类型，16位）
    const uint16_t VW_CYLINDER_PRESSURE = 100;  // 刚柔缸下降停止压力值
    const uint16_t VW_PLATFORM1_PRESSURE = 104; // 升降平台1上升停止压力值
    const uint16_t VW_PLATFORM2_PRESSURE = 108; // 升降平台2上升停止压力值
    const uint16_t VW_TILT_ANGLE = 112;        // 平台倾斜角度
    const uint16_t VW_POSITION = 116;          // 平台位置信息
}

/**
 * @struct DeviceState
 * @brief 设备状态数据结构体
 * @details 存储PLC原始数据和解析后的可读状态，用于API响应和状态展示
 * @note 包含便捷的访问函数，方便根据PLC地址直接操作原始数据
 */
struct DeviceState {
    /**
     * @struct RawData
     * @brief PLC原始数据存储结构
     * @details 按照VB和VW地址类型分别存储原始二进制数据
     */
    struct RawData {
        uint8_t vb_data[2000] = {0};   // VB地址范围的原始数据 (字节类型)
        int16_t vw_data[200] = {0};    // VW地址范围的原始数据 (字类型)
    } raw;
    
    // 解析后的状态信息（用于API响应和显示）
    std::string operationMode;     // 操作模式："手动"/"自动"
    std::string emergencyStop;     // 急停状态："复位"/"急停"
    std::string oilPumpStatus;     // 油泵状态："停止"/"启动"
    std::string cylinderState;     // 刚柔缸状态："下降停止"/"下降加压"/"上升停止"/"上升"
    std::string platform1State;    // 升降平台1状态："上升"/"上升停止"/"下降"/"下降停止"
    std::string platform2State;    // 升降平台2状态："上升"/"上升停止"/"下降"/"下降停止"
    std::string heaterStatus;      // 电加热状态："停止"/"启动"
    std::string coolingStatus;     // 风冷状态："停止"/"启动"
    std::string alarmStatus;       // 报警状态信息："油温低"/"油温高"/"液位低"/"液位高"/"滤芯堵"
    std::string levelingStatus;    // 电动缸调平："停止"/"启动"
    
    // 解析后的数值指标
    double cylinderPressure;       // 刚柔缸下降停止压力值
    double platform1Pressure;      // 升降平台1上升停止压力值
    double platform2Pressure;      // 升降平台2上升停止压力值
    double tiltAngle;              // 平台倾斜角度
    int platformPosition;          // 平台位置信息
    
    /**
     * @brief 获取VB地址对应的字节值
     * @param address PLC的VB地址
     * @return 对应地址的字节值
     */
    inline uint8_t getVB(uint16_t address) const { 
        return raw.vb_data[address]; 
    }
    
    /**
     * @brief 获取VW地址对应的字值
     * @param address PLC的VW地址
     * @return 对应地址的字值
     */
    inline int16_t getVW(uint16_t address) const { 
        return raw.vw_data[address]; 
    }
    
    /**
     * @brief 设置VB地址对应的字节值
     * @param address PLC的VB地址
     * @param value 要设置的值
     */
    inline void setVB(uint16_t address, uint8_t value) { 
        raw.vb_data[address] = value; 
    }
    
    /**
     * @brief 设置VW地址对应的字值
     * @param address PLC的VW地址
     * @param value 要设置的值
     */
    inline void setVW(uint16_t address, int16_t value) { 
        raw.vw_data[address] = value; 
    }
    
    /**
     * @brief 检查指定VB地址的指定位是否为1
     * @param address PLC的VB地址
     * @param bit_position 位位置(0-7)
     * @return 如果位为1返回true，否则返回false
     */
    inline bool isBitSet(uint16_t address, uint8_t bit_position) const {
        return (raw.vb_data[address] & (1 << bit_position)) != 0;
    }
    
    /**
     * @brief 设置指定VB地址的指定位
     * @param address PLC的VB地址
     * @param bit_position 位位置(0-7)
     * @param value 位值(true=1, false=0)
     */
    inline void setBit(uint16_t address, uint8_t bit_position, bool value) {
        if (value) {
            raw.vb_data[address] |= (1 << bit_position);
        } else {
            raw.vb_data[address] &= ~(1 << bit_position);
        }
    }
};

/**
 * @brief 状态转换工具函数 - 将设备状态转换为JSON字符串
 * @param state 设备状态对象
 * @return JSON格式的状态字符串
 */
std::string device_state_to_json(const DeviceState& state);