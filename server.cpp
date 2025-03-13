// server.cpp
#include "server.h"
#include <spdlog/spdlog.h>
#include "task_manager.h"
#include "common.h"
#include "plc_manager.h"

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

StabilityServer::StabilityServer(const utility::string_t& url) : m_listener(url) {
    init_routes();
}

void StabilityServer::init_routes() {
    // 统一处理GET请求
    m_listener.support(methods::GET, [this](http_request request) {
        const auto path = request.relative_uri().path();
        SPDLOG_DEBUG("[GET] 请求路径: {}", path);

        if (path == "/stability/health") {
            handle_health(request);
        }
        else if (path == "/stability/device/state") {
            handle_device_state(request);
        }
        else {
            request.reply(status_codes::NotFound);
        }
        });

    // 统一处理POST请求
    m_listener.support(methods::POST, [this](http_request request) {
        const auto path = request.relative_uri().path();
        SPDLOG_DEBUG("[POST] 请求路径: {}", path);

        if (path == "/stability/support/control") {
            handle_support_control(request);
        }
        else if (path == "/stability/platform/control") {
            handle_platform_control(request);
        }
        else if (path == "/stability/error/report") {
            handle_error_report(request);
        }
        else {
            request.reply(status_codes::NotFound);
        }
        });

    // 处理其他HTTP方法
    m_listener.support([](http_request request) {
        SPDLOG_WARN("不支持的HTTP方法: {}", request.method());
        request.reply(status_codes::MethodNotAllowed);
        });
}

// 健康检查处理
void StabilityServer::handle_health(http_request request) {
    SPDLOG_DEBUG("处理健康检查请求");
    json::value response;
    response["status"] = json::value::string("online");
    response["version"] = json::value::string(constants::VERSION);
    request.reply(status_codes::OK, response);
}

// 支撑控制处理
void StabilityServer::handle_support_control(http_request request) {
    request.extract_json()
        .then([=](json::value body) {
        try {
            // 参数校验
            if (!body.has_field("taskId") ||
                !body.has_field("defectId") ||
                !body.has_field("state")) {
                SPDLOG_WARN("支撑控制请求参数不完整");
                request.reply(status_codes::BadRequest);
                return;
            }

            const int taskId = body["taskId"].as_integer();
            const int defectId = body["defectId"].as_integer();
            const utility::string_t state = body["state"].as_string();

            SPDLOG_INFO("收到支撑控制请求 taskId={}, state={}", taskId, state);

            // 创建异步任务
            TaskManager::instance().create_task(taskId, defectId, state);

            json::value response;
            response["msg"] = json::value::string("操作已接受");
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("支撑控制请求解析失败: {}", e.what());
            request.reply(status_codes::BadRequest);
        }
            });
}

// 平台控制处理
void StabilityServer::handle_platform_control(http_request request) {
    request.extract_json()
        .then([=](json::value body) {
        try {
            // 参数校验
            if (!body.has_field("taskId") ||
                !body.has_field("defectId") ||
                !body.has_field("platformNum") ||
                !body.has_field("state")) {
                SPDLOG_WARN("平台控制请求参数不完整");
                request.reply(status_codes::BadRequest);
                return;
            }

            const int taskId = body["taskId"].as_integer();
            const int defectId = body["defectId"].as_integer();
            const int platformNum = body["platformNum"].as_integer();
            const utility::string_t state = body["state"].as_string();

            SPDLOG_INFO("收到平台控制请求 platform={}, state={}",
                platformNum, state);

            // 创建异步任务（带平台编号）
            TaskManager::instance().create_task(
                taskId,
                defectId,
                state,
                std::to_string(platformNum)
            );

            json::value response;
            response["msg"] = json::value::string("平台操作已接受");
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("平台控制请求解析失败: {}", e.what());
            request.reply(status_codes::BadRequest);
        }
            });
}

// 设备状态获取
void StabilityServer::handle_device_state(http_request request) {
    try {
        DeviceState state = PLCManager::instance().get_current_state();
        request.reply(status_codes::OK, device_state_to_json(state));
    }
    catch (const std::exception& e) {
        SPDLOG_ERROR("获取设备状态失败: {}", e.what());
        json::value error;
        error["error"] = json::value::string("DEVICE_UNAVAILABLE");
        request.reply(status_codes::InternalError, error);
    }
}

// 错误上报处理
void StabilityServer::handle_error_report(http_request request) {
    request.extract_json()
        .then([=](json::value body) {
        try {
            if (!body.has_field("alarm")) {
                SPDLOG_WARN("错误上报缺少alarm字段");
                request.reply(status_codes::BadRequest);
                return;
            }

            const auto alarm = body["alarm"].as_string();

            SPDLOG_WARN("收到系统报警: {}", alarm);

            json::value response;
            response["msg"] = json::value::string("报警已记录");
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("错误上报处理失败: {}", e.what());
            request.reply(status_codes::BadRequest);
        }
            });
}