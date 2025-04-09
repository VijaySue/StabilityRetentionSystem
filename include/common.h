/**
 * @file common.h
 * @brief 公共类和常量定义
 * @details 定义系统中使用的公共数据结构和常量
 * @author VijaySue
 * @date 2024-3-29
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
 * @note 更新地址映射：VB=字节(8位) VD=双字(32位,float)
 */
namespace plc_address {
    // VB1000 位定义（布尔类型）
    const uint16_t VB_CONTROL_BYTE = 1000;    // 控制字节，包含多个布尔值
    const uint8_t BIT_OPERATION_MODE = 0;     // 自动按钮（操作）: 1=自动, 0=手动
    const uint8_t BIT_EMERGENCY_STOP = 1;     // 急停按钮: 1=正常, 0=急停
    const uint8_t BIT_OIL_PUMP = 2;           // 油泵状态: 1=启动, 0=停止
    const uint8_t BIT_HEATER = 3;             // 电加热状态: 1=加热, 0=停止
    const uint8_t BIT_AIR_COOLING = 4;        // 风冷状态: 1=启动, 0=停止
    const uint8_t BIT_LEVELING1 = 5;          // 1#电动缸调平: 1=启动, 0=停止
    const uint8_t BIT_LEVELING2 = 6;          // 2#电动缸调平: 1=启动, 0=停止

    // 状态字节定义
    const uint16_t VB_CYLINDER_STATE = 1001;  // 刚柔缸状态: 1=下降停止, 2=下降加压, 4=上升停止, 8=上升加压
    const uint16_t VB_LIFT_PLATFORM1 = 1002;  // 升降平台1状态: 1=上升, 2=上升停止, 4=下降, 8=下降停止
    const uint16_t VB_LIFT_PLATFORM2 = 1003;  // 升降平台2状态: 1=上升, 2=上升停止, 4=下降, 8=下降停止
    
    // 报警信号地址定义
    const uint16_t VB_ALARM_OIL_TEMP = 1004;   // 油温报警: 1=油温低, 2=油温高, 4=正常
    const uint16_t VB_ALARM_LIQUID_LEVEL = 1005; // 液位报警: 1=液位低, 2=液位高, 4=正常
    const uint16_t VB_ALARM_FILTER = 1006;    // 滤芯堵报警: 1=滤芯堵, 2=正常
    
    // 保留原有常量以兼容现有代码
    const uint16_t VB_ALARM = VB_ALARM_OIL_TEMP; // 报警信号兼容旧代码
    
    // VD 地址（双字类型，32位浮点）
    const uint16_t VD_CYLINDER_PRESSURE = 1010;  // 刚柔缸下降停止压力值
    const uint16_t VD_LIFT_PRESSURE = 1014;      // 升降平台上升停止压力值
    const uint16_t VD_PLATFORM1_TILT = 1018;     // 平台1倾斜角度
    const uint16_t VD_PLATFORM2_TILT = 1022;     // 平台2倾斜角度
    const uint16_t VD_PLATFORM1_POS = 1026;      // 平台1位置信息
    const uint16_t VD_PLATFORM2_POS = 1030;      // 平台2位置信息
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
     * @details 按照VB和VD地址类型分别存储原始二进制数据
     */
    struct RawData {
        uint8_t vb_data[2000] = {0};   // VB地址范围的原始数据 (字节类型)
        float vd_data[2000] = {0};   // VD地址范围的原始数据 (32位浮点数)
    } raw;
    
    // 解析后的状态信息（用于API响应和显示）
    std::string operationMode;     // 操作模式："自动"/"手动"
    std::string emergencyStop;     // 急停状态："正常"/"急停"
    std::string oilPumpStatus;     // 油泵状态："启动"/"停止"
    std::string cylinderState;     // 刚柔缸状态："下降停止"/"下降加压"/"上升停止"/"上升加压"
    std::string platform1State;    // 升降平台1状态："上升"/"上升停止"/"下降"/"下降停止"
    std::string platform2State;    // 升降平台2状态："上升"/"上升停止"/"下降"/"下降停止"
    std::string heaterStatus;      // 电加热状态："加热"/"停止"
    std::string coolingStatus;     // 风冷状态："启动"/"停止"
    
    // 电动缸调平状态
    std::string leveling1Status;   // 1#电动缸调平："停止"/"启动"
    std::string leveling2Status;   // 2#电动缸调平："停止"/"启动"
    
    // 解析后的数值指标
    float cylinderPressure;        // 刚柔缸下降停止压力值
    float liftPressure;            // 升降平台上升停止压力值
    float platform1TiltAngle;      // 平台1倾斜角度
    float platform2TiltAngle;      // 平台2倾斜角度
    float platform1Position;       // 平台1位置信息
    float platform2Position;       // 平台2位置信息

    /**
     * @brief 获取VB地址对应的字节值
     * @param address PLC的VB地址
     * @return 对应地址的字节值
     */
    inline uint8_t getVB(uint16_t address) const { 
        return raw.vb_data[address]; 
    }
    
    /**
     * @brief 获取VD地址对应的浮点数值
     * @param address PLC的VD地址
     * @return 对应地址的浮点数值
     */
    inline float getVD(uint16_t address) const { 
        return raw.vd_data[address]; 
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
     * @brief 设置VD地址对应的浮点数值
     * @param address PLC的VD地址
     * @param value 要设置的值
     */
    inline void setVD(uint16_t address, float value) { 
        raw.vd_data[address] = value; 
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
