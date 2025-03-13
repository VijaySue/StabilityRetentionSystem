// callback_client.cpp
#include "callback_client.h"
#include <spdlog/spdlog.h>

CallbackClient& CallbackClient::instance() {
    static CallbackClient instance;
    return instance;
}

CallbackClient::CallbackClient()
    : m_client(EDGE_CALLBACK_URL) {  // 初始化边缘系统地址
    SPDLOG_DEBUG("初始化回调客户端，目标地址: {}", EDGE_CALLBACK_URL);
}

void CallbackClient::send_callback(int taskId, int defectId, const std::string& state) {
    web::json::value body;
    body["taskId"] = taskId;
    body["defectId"] = defectId;
    body["state"] = web::json::value::string(state);

    // 发送POST请求到边缘系统回调接口
    m_client.request(web::http::methods::POST, "/stability/support/cback", body)
        .then([=](web::http::http_response response) {
        if (response.status_code() != web::http::status_codes::OK) {
            SPDLOG_ERROR("回调失败，任务ID: {}，状态码: {}", taskId, response.status_code());
            return;
        }
        SPDLOG_INFO("成功发送回调，任务ID: {}，状态: {}", taskId, state);
            })
        .then([=](pplx::task<void> previousTask) {
        try {
            previousTask.get();
        }
        catch (const std::exception& e) {
            SPDLOG_CRITICAL("回调异常，任务ID: {}，错误: {}", taskId, e.what());
        }
            });
}