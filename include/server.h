/**
 * @file server.h
 * @brief HTTP服务器类定义
 * @details 提供RESTful API接口，实现系统状态检测、控制操作和状态获取等功能
 * @author VijaySue
 * @date 2024-3-11
 */
#pragma once
#include <cpprest/http_listener.h>
#include <memory>

/**
 * @brief HTTP服务端，实现稳定性系统所有RESTful API接口
 * @details 封装处理HTTP请求的各种逻辑，包括系统状态检测、设备控制、数据获取等
 */
class StabilityServer {
public:
    /**
     * @brief 构造函数
     * @param url 服务器监听的URL
     */
    explicit StabilityServer(const utility::string_t& url);

    /**
     * @brief 启动HTTP服务
     * @return 异步任务
     */
    pplx::task<void> open() { return m_listener.open(); }
    
    /**
     * @brief 关闭HTTP服务
     * @return 异步任务
     */
    pplx::task<void> close() { return m_listener.close(); }

private:
    /**
     * @brief 初始化路由配置
     * @details 设置不同URL路径与处理函数的映射关系
     */
    void init_routes();

    // 接口处理函数
    /**
     * @brief 系统状态检测接口
     * @param request HTTP请求对象
     * @details 处理 /stability/system/status GET请求，检测系统是否在线
     */
    void handle_health(web::http::http_request request);
    
    /**
     * @brief 刚性支撑/柔性复位接口
     * @param request HTTP请求对象
     * @details 处理 /stability/support/control POST请求，控制刚性支撑或柔性复位
     */
    void handle_support_control(web::http::http_request request);
    
    /**
     * @brief 作业平台升高/复位接口
     * @param request HTTP请求对象
     * @details 处理 /stability/platformHeight/control POST请求，控制作业平台升高或复位
     */
    void handle_platform_height_control(web::http::http_request request);
    
    /**
     * @brief 作业平台调平/调平复位接口
     * @param request HTTP请求对象
     * @details 处理 /stability/platformHorizontal/control POST请求，控制作业平台调平或调平复位
     */
    void handle_platform_horizontal_control(web::http::http_request request);
    
    /**
     * @brief 实时状态获取接口
     * @param request HTTP请求对象
     * @details 处理 /stability/device/state GET请求，获取系统所有设备状态
     */
    void handle_device_state(web::http::http_request request);

    web::http::experimental::listener::http_listener m_listener; // 监听器
};