// callback_client.h
#pragma once  
#include <cpprest/http_client.h>
#include <string>

/**
 * @brief 回调客户端单例，用于向边缘控制系统发送回调通知
 * @note 根据文档3.1-3.3章节回调接口规范实现
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

private:
    CallbackClient();  // 私有构造函数确保单例
    web::http::client::http_client m_client;  // HTTP客户端
    static const utility::string_t EDGE_CALLBACK_BASE_URL;  // 边缘系统基础地址
};