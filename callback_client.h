// callback_client.h
#pragma once  
#include <cpprest/http_client.h>  

/**
 * @brief 回调客户端单例，用于向边缘控制系统发送回调通知
 * @note 根据文档3.1-3.3章节回调接口规范实现
 */
class CallbackClient {
public:
    static CallbackClient& instance();

    /**
     * @brief 发送操作完成回调
     * @param taskId 任务ID（对应文档中的taskId参数）
     * @param defectId 缺陷ID（对应文档中的defectId参数）
     * @param state 状态信息（"已刚性支撑"/"已升高"等文档定义的状态）
     */
    void send_callback(int taskId, int defectId, const std::string& state);

private:
    CallbackClient();  // 私有构造函数确保单例
    web::http::client::http_client m_client;  // HTTP客户端
    static constexpr const char* EDGE_CALLBACK_URL = "http://edge-system/api/stability/callback";  // 边缘系统基础地址
};