/**
 * @file server.cpp
 * @brief RESTful API服务器实现
 * @details 提供HTTP接口以获取设备状态和控制设备操作
 * @author VijaySue
 * @version 2.0
 * @date 2024-3-11
 */
#include "../include/server.h"
#include <spdlog/spdlog.h>
#include "../include/task_manager.h"
#include "../include/common.h"
#include "../include/plc_manager.h"
#include <nlohmann/json.hpp>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

/**
 * @brief 获取系统平台信息
 * @return 平台信息字符串
 */
std::string get_platform_info() {
    #ifdef _WIN32
        return "Windows";
    #elif __linux__
        return "Linux";
    #elif __APPLE__
        return "macOS";
    #else
        return "Unknown Platform";
    #endif
}

/**
 * @brief 获取libmodbus库版本
 * @return 版本信息字符串
 */
std::string get_libmodbus_version() {
    // libmodbus没有直接提供版本信息的API，返回编译时信息
    return "libmodbus (编译时版本)";
}

/**
 * @brief 创建错误响应JSON
 * @param message 错误信息
 * @return JSON错误响应
 */
web::json::value create_error_response(const std::string& message) {
    web::json::value response;
    response["msg"] = web::json::value::string("error");
    response["error"] = web::json::value::string(message);
    return response;
}

/**
 * @brief 构造函数 - 初始化HTTP监听器
 * @param url HTTP监听URL
 */
StabilityServer::StabilityServer(const utility::string_t& url) : m_listener(url) {
    init_routes();
}

/**
 * @brief 初始化路由规则
 * @details 注册不同HTTP路径的处理函数
 */
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
        else if (path == "/stability/system/info") {
            handle_system_info(request);
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
        else if (path == "/stability/power/control") {
            handle_power_control(request);
        }
        else if (path == "/stability/motor/control") {
            handle_motor_control(request);
        }
        else if (path == "/stability/operation/mode") {
            handle_operation_mode(request);
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

/**
 * @brief 健康检查处理
 * @param request HTTP请求对象
 */
void StabilityServer::handle_health(http_request request) {
    SPDLOG_DEBUG("处理健康检查请求");
    web::json::value response;
    response["status"] = web::json::value::string("online");
    response["version"] = web::json::value::string(constants::VERSION);
    response["timestamp"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    request.reply(status_codes::OK, response);
}

/**
 * @brief 系统信息处理
 * @param request HTTP请求对象
 */
void StabilityServer::handle_system_info(http_request request) {
    try {
        web::json::value response;
        response["msg"] = web::json::value::string("success");
        response["version"] = web::json::value::string(constants::VERSION);
        response["buildTime"] = web::json::value::string(__DATE__ " " __TIME__);
        response["platform"] = web::json::value::string(get_platform_info());
        response["libmodbus"] = web::json::value::string(get_libmodbus_version());
        
        // 访问PLC配置信息 - 现在PLCManager中这些成员是公开的
        response["plcHost"] = web::json::value::string(PLCManager::PLC_IP_ADDRESS);
        response["plcPort"] = web::json::value::number(PLCManager::PLC_PORT);
        
        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e) {
        SPDLOG_ERROR("获取系统信息时发生异常: {}", e.what());
        request.reply(web::http::status_codes::InternalError, create_error_response("获取系统信息失败"));
    }
}

/**
 * @brief 支撑控制处理
 * @param request HTTP请求对象
 * @details 处理刚性支撑/柔性复位控制请求
 *          请求参数：taskId, defectId, state
 *          state取值：rigid(刚性支撑), flexible(柔性复位)
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
                
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("请求参数不完整，需要taskId, defectId和state字段");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }

            const int taskId = body["taskId"].as_integer();
            const int defectId = body["defectId"].as_integer();
            const utility::string_t state = body["state"].as_string();

            SPDLOG_INFO("支撑控制请求参数：taskId={}, defectId={}, state={}", 
                         taskId, defectId, state);

            // 验证state参数的有效性
            if (state != "rigid" && state != "flexible") {
                SPDLOG_WARN("无效的支撑控制状态: {}", state);
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("无效的state值，必须为'rigid'或'flexible'");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }
            
            // 创建异步任务（直接使用接口定义的操作名称）
            TaskManager::instance().create_task(taskId, defectId, state);

            // 返回成功响应
            web::json::value response;
            response["msg"] = web::json::value::string(constants::MSG_SUCCESS);
            response["taskId"] = web::json::value::number(taskId);
            response["defectId"] = web::json::value::number(defectId);
            response["state"] = web::json::value::string(state);
            response["status"] = web::json::value::string("processing");
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("支撑控制请求处理失败: {}", e.what());
            web::json::value error_response;
            error_response["msg"] = web::json::value::string("error");
            error_response["error"] = web::json::value::string(e.what());
            request.reply(status_codes::BadRequest, error_response);
        }
            });
}

/**
 * @brief 作业平台升高/降低控制
 * @param request HTTP请求对象
 * @details 处理平台升高/降低控制请求
 *          请求参数：taskId, defectId, platformNum, state
 *          state取值：up(上升), down(下降)
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
                
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("请求参数不完整，需要taskId, defectId, platformNum和state字段");
                request.reply(status_codes::BadRequest, error_response);
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
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("无效的platformNum值，必须为1或2");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }
            
            // 验证state参数的有效性
            if (state != "up" && state != "down") {
                SPDLOG_WARN("无效的平台控制状态: {}", state);
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("无效的state值，必须为'up'或'down'");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }
            
            // 创建异步任务（直接使用接口定义的操作名称，平台编号作为target）
            TaskManager::instance().create_task(
                taskId,
                defectId,
                state,
                std::to_string(platformNum)
            );

            // 返回成功响应
            web::json::value response;
            response["msg"] = web::json::value::string(constants::MSG_SUCCESS);
            response["taskId"] = web::json::value::number(taskId);
            response["defectId"] = web::json::value::number(defectId);
            response["platformNum"] = web::json::value::number(platformNum);
            response["state"] = web::json::value::string(state);
            response["status"] = web::json::value::string("processing");
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("平台高度控制请求处理失败: {}", e.what());
            web::json::value error_response;
            error_response["msg"] = web::json::value::string("error");
            error_response["error"] = web::json::value::string(e.what());
            request.reply(status_codes::BadRequest, error_response);
        }
            });
}

/**
 * @brief 作业平台调平/调平复位控制
 * @param request HTTP请求对象
 * @details 处理平台调平/调平复位控制请求
 *          请求参数：taskId, defectId, platformNum, state
 *          state取值：level(调平), level_reset(复位)
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
                
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("请求参数不完整，需要taskId, defectId, platformNum和state字段");
                request.reply(status_codes::BadRequest, error_response);
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
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("无效的platformNum值，必须为1或2");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }
            
            // 验证state参数的有效性
            if (state != "level" && state != "level_reset") {
                SPDLOG_WARN("无效的调平控制状态: {}", state);
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("无效的state值，必须为'level'或'level_reset'");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }
            
            // 创建异步任务（直接使用接口定义的操作名称，平台编号作为target）
            TaskManager::instance().create_task(
                taskId,
                defectId,
                state,
                std::to_string(platformNum)
            );

            // 返回成功响应
            web::json::value response;
            response["msg"] = web::json::value::string(constants::MSG_SUCCESS);
            response["taskId"] = web::json::value::number(taskId);
            response["defectId"] = web::json::value::number(defectId);
            response["platformNum"] = web::json::value::number(platformNum);
            response["state"] = web::json::value::string(state);
            response["status"] = web::json::value::string("processing");
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("平台调平控制请求处理失败: {}", e.what());
            web::json::value error_response;
            error_response["msg"] = web::json::value::string("error");
            error_response["error"] = web::json::value::string(e.what());
            request.reply(status_codes::BadRequest, error_response);
        }
            });
}

/**
 * @brief 电源控制处理
 * @param request HTTP请求对象
 * @details 处理电加热电源的开关控制请求
 *          请求参数：state
 *          state取值：on(开启), off(关闭)
 */
void StabilityServer::handle_power_control(http_request request) {
    request.extract_json()
        .then([=](web::json::value body) {
        try {
            SPDLOG_INFO("收到电源控制请求");
            
            // 参数校验
            if (!body.has_field("state")) {
                SPDLOG_WARN("电源控制请求参数不完整");
                
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("请求参数不完整，需要state字段");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }

            const utility::string_t state = body["state"].as_string();
            SPDLOG_INFO("电源控制请求参数：state={}", state);

            // 验证state参数的有效性
            if (state != "on" && state != "off") {
                SPDLOG_WARN("无效的电源控制状态: {}", state);
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("无效的state值，必须为'on'或'off'");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }
            
            // 映射操作名称
            std::string operation = (state == "on") ? "power_on" : "power_off";
            
            // 执行电源控制操作（无需异步处理，直接执行）
            PLCManager::instance().execute_operation(operation);

            // 返回成功响应
            web::json::value response;
            response["msg"] = web::json::value::string(constants::MSG_SUCCESS);
            response["state"] = web::json::value::string(state);
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("电源控制请求处理失败: {}", e.what());
            web::json::value error_response;
            error_response["msg"] = web::json::value::string("error");
            error_response["error"] = web::json::value::string(e.what());
            request.reply(status_codes::BadRequest, error_response);
        }
            });
}

/**
 * @brief 电机控制处理
 * @param request HTTP请求对象
 * @details 处理油泵电机的启停控制请求
 *          请求参数：state
 *          state取值：start(启动), stop(停止)
 */
void StabilityServer::handle_motor_control(http_request request) {
    request.extract_json()
        .then([=](web::json::value body) {
        try {
            SPDLOG_INFO("收到电机控制请求");
            
            // 参数校验
            if (!body.has_field("state")) {
                SPDLOG_WARN("电机控制请求参数不完整");
                
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("请求参数不完整，需要state字段");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }

            const utility::string_t state = body["state"].as_string();
            SPDLOG_INFO("电机控制请求参数：state={}", state);

            // 验证state参数的有效性
            if (state != "start" && state != "stop") {
                SPDLOG_WARN("无效的电机控制状态: {}", state);
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("无效的state值，必须为'start'或'stop'");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }
            
            // 映射操作名称
            std::string operation = (state == "start") ? "motor_start" : "motor_stop";
            
            // 执行电机控制操作（无需异步处理，直接执行）
            PLCManager::instance().execute_operation(operation);

            // 返回成功响应
            web::json::value response;
            response["msg"] = web::json::value::string(constants::MSG_SUCCESS);
            response["state"] = web::json::value::string(state);
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("电机控制请求处理失败: {}", e.what());
            web::json::value error_response;
            error_response["msg"] = web::json::value::string("error");
            error_response["error"] = web::json::value::string(e.what());
            request.reply(status_codes::BadRequest, error_response);
        }
            });
}

/**
 * @brief 操作模式控制
 * @param request HTTP请求对象
 * @details 处理系统操作模式设置请求
 *          请求参数：mode
 *          mode取值：auto(自动), manual(手动)
 */
void StabilityServer::handle_operation_mode(http_request request) {
    request.extract_json()
        .then([=](web::json::value body) {
        try {
            SPDLOG_INFO("收到操作模式控制请求");
            
            // 参数校验
            if (!body.has_field("mode")) {
                SPDLOG_WARN("操作模式请求参数不完整");
                
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("请求参数不完整，需要mode字段");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }

            const utility::string_t mode = body["mode"].as_string();
            SPDLOG_INFO("操作模式控制请求参数：mode={}", mode);

            // 验证mode参数的有效性
            if (mode != "auto" && mode != "manual") {
                SPDLOG_WARN("无效的操作模式: {}", mode);
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("无效的mode值，必须为'auto'或'manual'");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }
            
            // 执行操作模式设置（无需异步，直接执行）
            PLCManager::instance().execute_operation(mode);

            // 返回成功响应
            web::json::value response;
            response["msg"] = web::json::value::string(constants::MSG_SUCCESS);
            response["mode"] = web::json::value::string(mode);
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("操作模式设置请求处理失败: {}", e.what());
            web::json::value error_response;
            error_response["msg"] = web::json::value::string("error");
            error_response["error"] = web::json::value::string(e.what());
            request.reply(status_codes::BadRequest, error_response);
        }
            });
}

/**
 * @brief 设备状态获取
 * @param request HTTP请求对象
 */
void StabilityServer::handle_device_state(http_request request) {
    try {
        // 获取查询参数
        auto query = request.relative_uri().query();
        auto query_params = web::uri::split_query(query);
        
        // 获取设备状态
        DeviceState state = PLCManager::instance().get_current_state();
        
        // 使用新的转换函数生成JSON字符串
        std::string json_str = device_state_to_json(state);
        
        // 根据参数筛选响应内容
        if (!query_params.empty()) {
            // 如果有查询参数，可以解析json_str并进行过滤
            auto json_obj = nlohmann::json::parse(json_str);
            
            // 检查是否有指定需要返回的字段
            if (query_params.find(U("fields")) != query_params.end()) {
                auto fields_param = query_params[U("fields")];
                std::vector<std::string> fields;
                
                // 分割字段参数
                std::istringstream iss(utility::conversions::to_utf8string(fields_param));
                std::string field;
                while (std::getline(iss, field, ',')) {
                    fields.push_back(field);
                }
                
                if (!fields.empty()) {
                    // 创建一个只包含指定字段的新JSON对象
                    nlohmann::json filtered_json;
                    
                    // 保留msg字段和timestamp作为基本响应
                    if (json_obj.contains("msg")) {
                        filtered_json["msg"] = json_obj["msg"];
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
        
        // 创建HTTP响应
        request.reply(status_codes::OK, json_str, "application/json");
        
        SPDLOG_DEBUG("设备状态请求已处理");
    }
    catch (const std::exception& e) {
        SPDLOG_ERROR("设备状态请求处理失败: {}", e.what());
        
        web::json::value error_response;
        error_response["msg"] = web::json::value::string("error");
        error_response["error"] = web::json::value::string(e.what());
        
        request.reply(status_codes::InternalError, error_response);
    }
}

/**
 * @brief 错误上报处理
 * @param request HTTP请求对象
 * @details 处理系统错误和异常上报
 *          请求参数：alarm - 报警信息
 */
void StabilityServer::handle_error_report(http_request request) {
    request.extract_json()
        .then([=](web::json::value body) {
        try {
            SPDLOG_INFO("收到错误上报请求");
            
            // 参数校验
            if (!body.has_field("alarm")) {
                SPDLOG_WARN("错误上报请求参数不完整");
                
                web::json::value error_response;
                error_response["msg"] = web::json::value::string("error");
                error_response["error"] = web::json::value::string("请求参数不完整，需要alarm字段");
                request.reply(status_codes::BadRequest, error_response);
                return;
            }

            const std::string alarm = utility::conversions::to_utf8string(body["alarm"].as_string());
            SPDLOG_WARN("系统报警信息: {}", alarm);
            
            // 可选的额外信息
            std::string source = "unknown";
            std::string level = "warning";
            std::string timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
            
            if (body.has_field("source")) {
                source = utility::conversions::to_utf8string(body["source"].as_string());
            }
            
            if (body.has_field("level")) {
                level = utility::conversions::to_utf8string(body["level"].as_string());
            }
            
            // 记录完整的报警信息
            SPDLOG_ERROR("系统报警: 来源={}, 级别={}, 信息={}", source, level, alarm);
            
            // 返回成功响应
            web::json::value response;
            response["msg"] = web::json::value::string(constants::MSG_SUCCESS);
            response["alarm"] = web::json::value::string(alarm);
            response["timestamp"] = web::json::value::string(timestamp);
            request.reply(status_codes::OK, response);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("错误上报请求处理失败: {}", e.what());
            web::json::value error_response;
            error_response["msg"] = web::json::value::string("error");
            error_response["error"] = web::json::value::string(e.what());
            request.reply(status_codes::BadRequest, error_response);
        }
            });
}