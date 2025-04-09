/**
 * @file server.cpp
 * @brief 服务器实现
 * @details 处理HTTP请求，提供REST API接口
 * @author VijaySue
 * @date 2024-3-29
 */
#include "../include/server.h"
#include <spdlog/spdlog.h>
#include "../include/task_manager.h"
#include "../include/common.h"
#include "../include/plc_manager.h"
#include "../include/config_manager.h"
#include <nlohmann/json.hpp>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

/**
 * @brief 创建错误响应JSON
 * @param message 错误信息
 * @param code HTTP错误码
 * @return JSON错误响应对象
 * @details 创建标准格式的错误响应，包含msg、code和error字段
 */
web::json::value create_error_response(const std::string& message, int code = 400) {
    web::json::value response;
    response["msg"] = web::json::value::string("error");
    response["code"] = web::json::value::number(code);
    response["error"] = web::json::value::string(message);
    return response;
}

/**
 * @brief 构造函数 - 初始化HTTP监听器
 * @param url HTTP监听URL
 * @details 创建HTTP服务器并初始化路由规则
 */
StabilityServer::StabilityServer(const utility::string_t& url) : m_listener(url) {
    init_routes();
}

/**
 * @brief 初始化路由规则
 * @details 注册不同HTTP路径的处理函数，包括：
 *          - GET请求：健康检查、设备状态、系统信息
 *          - POST请求：支撑控制、平台控制、电源控制等
 */
void StabilityServer::init_routes() {
    // 统一处理GET请求
    m_listener.support(methods::GET, [this](http_request request) {
        const auto path = request.relative_uri().path();
        SPDLOG_DEBUG("[GET] 请求路径: {}", path);

        if (path == "/stability/system/status") {
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
        else if (path == "/stability/platformHeight/control") {
            handle_platform_height_control(request);
        }
        else if (path == "/stability/platformHorizontal/control") {
            handle_platform_horizontal_control(request);
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

/**
 * @brief 系统状态检测处理
 * @param request HTTP请求对象
 * @details 返回系统在线状态
 */
void StabilityServer::handle_health(http_request request) {
    SPDLOG_DEBUG("处理系统状态检测请求");
    web::json::value response;

    // 检查PLC连接状态
    PLCManager& plc = PLCManager::instance();

    // 先检查连接状态，避免在未连接状态调用get_current_state
    if (!plc.is_connected()) {
        SPDLOG_INFO("PLC未连接，返回错误响应");
        response["msg"] = web::json::value::string("error");
        response["code"] = web::json::value::number(503);
        response["state"] = web::json::value::string("offline");
        request.reply(status_codes::ServiceUnavailable, response);
    } else {
        // 只有当PLC已连接时，才尝试读取数据
        try {
            DeviceState state = plc.get_current_state();
            response["msg"] = web::json::value::string(constants::MSG_SUCCESS);
            response["code"] = web::json::value::number(200);
            response["state"] = web::json::value::string("online");
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("读取PLC数据失败: {}", e.what());
            response["msg"] = web::json::value::string("error");
            response["code"] = web::json::value::number(503);
            request.reply(status_codes::ServiceUnavailable, response);
        }
    }
}

/**
 * @brief 支撑控制处理
 * @param request HTTP请求对象
 * @details 处理刚性支撑/柔性复位控制请求
 *          请求参数：
 *          - taskId: 任务ID
 *          - defectId: 缺陷ID
 *          - state: 支撑状态(rigid/flexible)
 */
void StabilityServer::handle_support_control(http_request request) {
    request.extract_json()
        .then([=](web::json::value body) {
        try {
            SPDLOG_INFO("收到支撑控制请求");

            // 参数校验
            if (!body.has_field("taskId") ||
                !body.has_field("defectId") ||
                !body.has_field("state")) {
                SPDLOG_WARN("支撑控制请求参数不完整");
                
                request.reply(status_codes::BadRequest, 
                    create_error_response("请求参数不完整，需要taskId, defectId和state字段", 400));
                return;
            }

            const int taskId = body["taskId"].as_integer();
            const int defectId = body["defectId"].as_integer();
            const utility::string_t state = body["state"].as_string();

            SPDLOG_INFO("支撑控制请求参数：taskId={}, defectId={}, state={}",
                taskId, defectId, state);

            // 验证state参数的有效性
            if (state != "刚性支撑" && state != "柔性复位") {
                SPDLOG_WARN("无效的支撑控制状态: {}", state);
                
                request.reply(status_codes::BadRequest,
                    create_error_response("无效的state值，必须为'刚性支撑'或'柔性复位'", 400));
                return;
            }

            // 检查PLC连接状态
            PLCManager& plc = PLCManager::instance();
            if (!plc.is_connected()) {
                SPDLOG_WARN("PLC未连接，无法执行支撑控制操作");
                
                request.reply(status_codes::ServiceUnavailable,
                    create_error_response("PLC设备未连接，无法执行操作", 503));
                return;
            }

            // 创建异步任务
            // JSON映射: state → operation
            // JSON中没有platformNum，所以不提供target参数
            TaskManager::instance().create_task(taskId, defectId, state);  // state作为operation参数

            // 返回成功响应
            web::json::value response;
            response["msg"] = web::json::value::string(constants::MSG_SUCCESS);
            response["code"] = web::json::value::number(200);
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("支撑控制请求处理失败: {}", e.what());
            
            request.reply(status_codes::BadRequest,
                create_error_response(e.what(), 400));
        }
            });
}

/**
 * @brief 作业平台升高/降低控制
 * @param request HTTP请求对象
 * @details 处理平台升高/降低控制请求
 *          请求参数：
 *          - taskId: 任务ID
 *          - defectId: 缺陷ID
 *          - platformNum: 平台编号(1/2)
 *          - state: 控制状态(up/down)
 */
void StabilityServer::handle_platform_height_control(http_request request) {
    request.extract_json()
        .then([=](web::json::value body) {
        try {
            SPDLOG_INFO("收到平台高度控制请求");

            // 参数校验
            if (!body.has_field("taskId") ||
                !body.has_field("defectId") ||
                !body.has_field("platformNum") ||
                !body.has_field("state")) {
                SPDLOG_WARN("平台高度控制请求参数不完整");
                
                request.reply(status_codes::BadRequest,
                    create_error_response("请求参数不完整，需要taskId, defectId, platformNum和state字段", 400));
                return;
            }

            const int taskId = body["taskId"].as_integer();
            const int defectId = body["defectId"].as_integer();
            const int platformNum = body["platformNum"].as_integer();
            const utility::string_t state = body["state"].as_string();

            SPDLOG_INFO("平台高度控制请求参数：taskId={}, defectId={}, platformNum={}, state={}",
                taskId, defectId, platformNum, state);

            // 验证platformNum参数的有效性
            if (platformNum != 1 && platformNum != 2) {
                SPDLOG_WARN("无效的平台编号: {}", platformNum);
                
                request.reply(status_codes::BadRequest,
                    create_error_response("无效的platformNum值，必须为1或2", 400));
                return;
            }

            // 验证state参数的有效性
            if (state != "升高" && state != "复位") {
                SPDLOG_WARN("无效的平台控制状态: {}", state);
                
                request.reply(status_codes::BadRequest,
                    create_error_response("无效的state值，必须为'升高'或'复位'", 400));
                return;
            }

            // 检查PLC连接状态
            PLCManager& plc = PLCManager::instance();
            if (!plc.is_connected()) {
                SPDLOG_WARN("PLC未连接，无法执行平台高度控制操作");
                
                request.reply(status_codes::ServiceUnavailable,
                    create_error_response("PLC设备未连接，无法执行操作", 503));
                return;
            }

            // 创建异步任务
            // JSON映射: state → operation, platformNum → target
            TaskManager::instance().create_task(
                taskId,                       // JSON中的taskId
                defectId,                     // JSON中的defectId
                state,                        // JSON中的state作为operation参数
                std::to_string(platformNum)   // JSON中的platformNum作为target参数
            );

            // 返回成功响应
            web::json::value response;
            response["msg"] = web::json::value::string(constants::MSG_SUCCESS);
            response["code"] = web::json::value::number(200);
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("平台高度控制请求处理失败: {}", e.what());
            
            request.reply(status_codes::BadRequest,
                create_error_response(e.what(), 400));
        }
            });
}

/**
 * @brief 作业平台调平/调平复位控制
 * @param request HTTP请求对象
 * @details 处理平台调平/调平复位控制请求
 *          请求参数：
 *          - taskId: 任务ID
 *          - defectId: 缺陷ID
 *          - platformNum: 平台编号(1/2)
 *          - state: 控制状态(level/level_reset)
 */
void StabilityServer::handle_platform_horizontal_control(http_request request) {
    request.extract_json()
        .then([=](web::json::value body) {
        try {
            SPDLOG_INFO("收到平台调平控制请求");

            // 参数校验
            if (!body.has_field("taskId") ||
                !body.has_field("defectId") ||
                !body.has_field("platformNum") ||
                !body.has_field("state")) {
                SPDLOG_WARN("平台调平控制请求参数不完整");
                
                request.reply(status_codes::BadRequest,
                    create_error_response("请求参数不完整，需要taskId, defectId, platformNum和state字段", 400));
                return;
            }

            const int taskId = body["taskId"].as_integer();
            const int defectId = body["defectId"].as_integer();
            const int platformNum = body["platformNum"].as_integer();
            const utility::string_t state = body["state"].as_string();

            SPDLOG_INFO("平台调平控制请求参数：taskId={}, defectId={}, platformNum={}, state={}",
                taskId, defectId, platformNum, state);

            // 验证platformNum参数的有效性
            if (platformNum != 1 && platformNum != 2) {
                SPDLOG_WARN("无效的平台编号: {}", platformNum);
                
                request.reply(status_codes::BadRequest,
                    create_error_response("无效的platformNum值，必须为1或2", 400));
                return;
            }

            // 验证state参数的有效性
            if (state != "调平" && state != "调平复位") {
                SPDLOG_WARN("无效的调平控制状态: {}", state);
                
                request.reply(status_codes::BadRequest,
                    create_error_response("无效的state值，必须为'调平'或'调平复位'", 400));
                return;
            }

            // 检查PLC连接状态
            PLCManager& plc = PLCManager::instance();
            if (!plc.is_connected()) {
                SPDLOG_WARN("PLC未连接，无法执行平台调平控制操作");
                
                request.reply(status_codes::ServiceUnavailable,
                    create_error_response("PLC设备未连接，无法执行操作", 503));
                return;
            }

            // 创建异步任务
            // JSON映射: state → operation, platformNum → target
            TaskManager::instance().create_task(
                taskId,                       // JSON中的taskId
                defectId,                     // JSON中的defectId
                state,                        // JSON中的state作为operation参数
                std::to_string(platformNum)   // JSON中的platformNum作为target参数
            );

            // 返回成功响应
            web::json::value response;
            response["msg"] = web::json::value::string(constants::MSG_SUCCESS);
            response["code"] = web::json::value::number(200);
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("平台调平控制请求处理失败: {}", e.what());
            
            request.reply(status_codes::BadRequest,
                create_error_response(e.what(), 400));
        }
            });
}

/**
 * @brief 设备状态获取
 * @param request HTTP请求对象
 * @details 获取当前设备状态信息，支持通过fields参数筛选返回字段
 *          查询参数：
 *          - fields: 指定返回的字段，多个字段用逗号分隔
 */
void StabilityServer::handle_device_state(http_request request) {
    try {
        // 获取查询参数
        auto query = request.relative_uri().query();
        auto query_params = web::uri::split_query(query);

        // 检查PLC连接状态
        PLCManager& plc = PLCManager::instance();
        if (!plc.is_connected()) {
            SPDLOG_INFO("PLC未连接，返回错误响应");
            web::json::value response;
            response["msg"] = web::json::value::string("error");
            response["code"] = web::json::value::number(503);
            response["timestamp"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
            
            request.reply(status_codes::ServiceUnavailable, response);
            return;
        }

        // 获取设备状态
        DeviceState state = plc.get_current_state();

        // 使用新的转换函数生成JSON字符串
        std::string json_str = device_state_to_json(state);

        // 根据参数筛选响应内容
        if (!query_params.empty()) {
            try {
                // 如果有查询参数，可以解析json_str并进行过滤
                nlohmann::json json_obj = nlohmann::json::parse(json_str);

                // 检查是否有指定需要返回的字段
                if (query_params.find(U("fields")) != query_params.end()) {
                    auto fields_param = query_params[U("fields")];
                    std::vector<std::string> fields;

                    // 分割字段参数
                    std::istringstream iss(utility::conversions::to_utf8string(fields_param));
                    std::string field;
                    while (std::getline(iss, field, ',')) {
                        // 清理字段名中的空白字符
                        field.erase(0, field.find_first_not_of(" \t\r\n"));
                        field.erase(field.find_last_not_of(" \t\r\n") + 1);
                        if (!field.empty()) {
                            fields.push_back(field);
                        }
                    }

                    if (!fields.empty()) {
                        // 创建一个只包含指定字段的新JSON对象
                        nlohmann::json filtered_json;

                        // 保留msg字段、code字段和timestamp作为基本响应
                        if (json_obj.contains("msg")) {
                            filtered_json["msg"] = json_obj["msg"];
                        }
                        if (json_obj.contains("code")) {
                            filtered_json["code"] = json_obj["code"];
                        }
                        if (json_obj.contains("timestamp")) {
                            filtered_json["timestamp"] = json_obj["timestamp"];
                        }

                        // 添加请求的字段
                        for (const auto& field : fields) {
                            if (json_obj.contains(field)) {
                                filtered_json[field] = json_obj[field];
                            }
                        }

                        // 使用过滤后的JSON
                        json_str = filtered_json.dump();
                    }
                }
            }
            catch (const nlohmann::json::exception& e) {
                SPDLOG_ERROR("JSON处理错误: {}", e.what());
                throw std::runtime_error("JSON处理错误: " + std::string(e.what()));
            }
        }

        // 创建HTTP响应
        request.reply(status_codes::OK, json_str, "application/json");

        SPDLOG_DEBUG("设备状态请求已处理");
    }
    catch (const std::exception& e) {
        SPDLOG_ERROR("设备状态请求处理失败: {}", e.what());

        web::json::value error_response = create_error_response("获取设备状态失败：" + std::string(e.what()), 500);
        request.reply(status_codes::InternalError, error_response);
    }
}