/**
 * @file callback_client.h
 * @brief 回调客户端类定义
 * @details 定义向边缘控制系统发送回调通知的功能
 * @author VijaySue
 * @date 2024-3-29
 */
#pragma once  
#include <cpprest/http_client.h>
#include <string>

/**
 * @brief 回调客户端单例，用于向边缘控制系统发送回调通知
 * @note 根据文档回调接口规范实现
 */
class CallbackClient {
public:
    static CallbackClient& instance();

    /**
     * @brief 发送支撑操作完成回调
     * @param taskId 任务ID（对应文档中的taskId参数）
     * @param defectId 缺陷ID（对应文档中的defectId参数）
     * @param state 状态信息（"已刚性支撑"/"已柔性复位"等文档定义的状态）
     */
    void send_support_callback(int taskId, int defectId, const std::string& state);
    
    /**
     * @brief 发送平台高度操作完成回调
     * @param taskId 任务ID（对应文档中的taskId参数）
     * @param defectId 缺陷ID（对应文档中的defectId参数）
     * @param platformNum 平台编号
     * @param state 状态信息（"已升高"/"已复位"等文档定义的状态）
     */
    void send_platform_height_callback(int taskId, int defectId, int platformNum, const std::string& state);
    
    /**
     * @brief 发送平台调平操作完成回调
     * @param taskId 任务ID（对应文档中的taskId参数）
     * @param defectId 缺陷ID（对应文档中的defectId参数）
     * @param platformNum 平台编号
     * @param state 状态信息（"已调平"/"已复位"等文档定义的状态）
     */
    void send_platform_horizontal_callback(int taskId, int defectId, int platformNum, const std::string& state);

    /**
     * @brief 发送系统报警信号
     * @param alarm_description 报警描述（"油温低"，"油温高"，"液位低"，"液位高"，"滤芯堵"等）
     * @details 使用文档规定的/stability/error/report接口上报报警信息
     */
    void send_alarm_callback(const std::string& alarm_description);

private:
    CallbackClient();  // 私有构造函数确保单例
    web::http::client::http_client m_client;  // HTTP客户端
    
    /**
     * @brief 获取边缘系统回调基础URL
     * @details 从ConfigManager获取边缘系统地址和端口，并构建完整URL
     * @return 边缘系统基础URL
     */
    static const utility::string_t get_edge_callback_base_url();
};