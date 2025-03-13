// server.h
#pragma once
#include <cpprest/http_listener.h>
#include <memory>

/**
 * @brief HTTP服务端，实现文档第3章所有接口
 */
class StabilityServer {
public:
    explicit StabilityServer(const utility::string_t& url);

    pplx::task<void> open() { return m_listener.open(); }
    pplx::task<void> close() { return m_listener.close(); }

private:
    void init_routes();  // 初始化路由配置

    // 接口处理函数
    void handle_health(web::http::http_request request);       // 3.1 系统状态检测
    void handle_support_control(web::http::http_request request); // 刚性支撑/柔性复位
    void handle_platform_control(web::http::http_request request); // 平台控制接口组
    void handle_device_state(web::http::http_request request); // 3.4 实时状态获取
    void handle_error_report(web::http::http_request request); // 3.5 错误上报

    web::http::experimental::listener::http_listener m_listener; // 监听器
};