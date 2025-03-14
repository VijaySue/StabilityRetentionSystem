/**
 * @file callback_client.cpp
 * @brief 回调客户端实现
 * @details 实现向边缘控制系统发送回调通知的功能
 * @author VijaySue
 * @date 2024-3-11
 */

// callback_client.cpp
#include "../include/callback_client.h"
#include <spdlog/spdlog.h>

const utility::string_t CallbackClient::EDGE_CALLBACK_BASE_URL = U("http://edge-system/api");

CallbackClient& CallbackClient::instance() {
    static CallbackClient instance;
    return instance;
}

CallbackClient::CallbackClient()
    : m_client(EDGE_CALLBACK_BASE_URL) {  // 初始化边缘系统地址
    SPDLOG_DEBUG("初始化回调客户端，目标地址: {}", utility::conversions::to_utf8string(EDGE_CALLBACK_BASE_URL));
}

void CallbackClient::send_support_callback(int taskId, int defectId, const std::string& state) {
    web::json::value body;
    body["taskId"] = taskId;
    body["defectId"] = defectId;
    body["state"] = web::json::value::string(state);

    // 发送POST请求到边缘系统回调接口
    m_client.request(web::http::methods::POST, "/stability/support/cback", body)
        .then([=](web::http::http_response response) {
        if (response.status_code() != web::http::status_codes::OK) {
            SPDLOG_ERROR("支撑回调失败，任务ID: {}，状态码: {}", taskId, response.status_code());
            return;
        }
        SPDLOG_INFO("成功发送支撑回调，任务ID: {}，状态: {}", taskId, state);
            })
        .then([=](pplx::task<void> previousTask) {
        try {
            previousTask.get();
        }
        catch (const std::exception& e) {
            SPDLOG_CRITICAL("支撑回调异常，任务ID: {}，错误: {}", taskId, e.what());
        }
            });
}

void CallbackClient::send_platform_height_callback(int taskId, int defectId, int platformNum, const std::string& state) {
    web::json::value body;
    body["taskId"] = taskId;
    body["defectId"] = defectId;
    body["platformNum"] = platformNum;
    body["state"] = web::json::value::string(state);

    // 发送POST请求到边缘系统回调接口
    m_client.request(web::http::methods::POST, "/stability/platformHeight/cback", body)
        .then([=](web::http::http_response response) {
        if (response.status_code() != web::http::status_codes::OK) {
            SPDLOG_ERROR("平台高度回调失败，任务ID: {}，状态码: {}", taskId, response.status_code());
            return;
        }
        SPDLOG_INFO("成功发送平台高度回调，任务ID: {}，平台: {}，状态: {}", taskId, platformNum, state);
            })
        .then([=](pplx::task<void> previousTask) {
        try {
            previousTask.get();
        }
        catch (const std::exception& e) {
            SPDLOG_CRITICAL("平台高度回调异常，任务ID: {}，错误: {}", taskId, e.what());
        }
            });
}

void CallbackClient::send_platform_horizontal_callback(int taskId, int defectId, int platformNum, const std::string& state) {
    web::json::value body;
    body["taskId"] = taskId;
    body["defectId"] = defectId;
    body["platformNum"] = platformNum;
    body["state"] = web::json::value::string(state);

    // 发送POST请求到边缘系统回调接口
    m_client.request(web::http::methods::POST, "/stability/platformHorizontal/cback", body)
        .then([=](web::http::http_response response) {
        if (response.status_code() != web::http::status_codes::OK) {
            SPDLOG_ERROR("平台调平回调失败，任务ID: {}，状态码: {}", taskId, response.status_code());
            return;
        }
        SPDLOG_INFO("成功发送平台调平回调，任务ID: {}，平台: {}，状态: {}", taskId, platformNum, state);
            })
        .then([=](pplx::task<void> previousTask) {
        try {
            previousTask.get();
        }
        catch (const std::exception& e) {
            SPDLOG_CRITICAL("平台调平回调异常，任务ID: {}，错误: {}", taskId, e.what());
        }
            });
}