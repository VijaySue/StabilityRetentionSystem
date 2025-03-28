/**
 * @file main.cpp
 * @brief 稳定性保持系统主程序入口
 * @author VijaySue
 * @date 2024-3-11
 */

// main.cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <cpprest/uri_builder.h>
#include "../include/server.h"
#include "../include/common.h"
#include "../include/config_manager.h"
#include "../include/plc_manager.h"
#include "../include/alarm_monitor.h"

// 信号处理
std::condition_variable_any shutdown_cv;
std::mutex shutdown_mutex;
bool shutdown_requested = false;

void signal_handler(int signal) {
    std::lock_guard<std::mutex> lock(shutdown_mutex);
    shutdown_requested = true;
    shutdown_cv.notify_all();
}

int main(int argc, char* argv[]) {
    // 初始化默认日志级别，以便能看到配置加载的日志
    spdlog::set_level(spdlog::level::info);

    // 加载配置文件
    std::string config_file = "config/config.ini";
    if (argc > 2) {
        config_file = argv[2];  // 允许通过命令行指定配置文件路径
    }
    
    SPDLOG_INFO("开始加载配置文件: {}", config_file);
    if (ConfigManager::instance().load_config(config_file)) {
        SPDLOG_INFO("成功加载配置文件");
        
        // 输出当前配置以便诊断
        auto& config = ConfigManager::instance();
        SPDLOG_INFO("配置值:");
        SPDLOG_INFO("  服务器主机: {}", config.get_server_host());
        SPDLOG_INFO("  服务器端口: {}", config.get_server_port());
        SPDLOG_INFO("  PLC IP地址: {}", config.get_plc_ip());
        SPDLOG_INFO("  PLC端口: {}", config.get_plc_port());
        SPDLOG_INFO("  边缘系统地址: {}", config.get_edge_system_address());
        SPDLOG_INFO("  边缘系统端口: {}", config.get_edge_system_port());
        SPDLOG_INFO("  日志级别: {}", config.get_log_level());
    } else {
        SPDLOG_ERROR("无法加载配置文件，将使用默认配置");
    }

    // 设置日志级别
    std::string log_level = ConfigManager::instance().get_log_level();
    if (log_level == "trace") spdlog::set_level(spdlog::level::trace);
    else if (log_level == "debug") spdlog::set_level(spdlog::level::debug);
    else if (log_level == "info") spdlog::set_level(spdlog::level::info);
    else if (log_level == "warning") spdlog::set_level(spdlog::level::warn);
    else if (log_level == "error") spdlog::set_level(spdlog::level::err);
    else if (log_level == "critical") spdlog::set_level(spdlog::level::critical);
    else spdlog::set_level(spdlog::level::info);

    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // 获取服务器配置
        std::string address = ConfigManager::instance().get_server_host();
        int port = ConfigManager::instance().get_server_port();

        // 如果命令行指定了端口，则使用命令行参数
        if (argc > 1) {
            port = std::stoi(argv[1]);
        }

        // 构建服务器URL
        utility::string_t url = web::uri_builder()
            .set_scheme(U("http"))
            .set_host(utility::conversions::to_string_t(address))
            .set_port(port)
            .to_uri()
            .to_string();

        SPDLOG_INFO("启动服务器: {}", utility::conversions::to_utf8string(url));

        // 创建并启动服务器
        StabilityServer server(url);
        server.open().wait();

        // 先尝试初始PLC连接
        SPDLOG_INFO("尝试初始PLC连接...");
        bool plc_connected = PLCManager::instance().connect_plc();
        if (plc_connected) {
            SPDLOG_INFO("初始PLC连接成功");
        } else {
            SPDLOG_ERROR("初始PLC连接失败，将通过报警监控系统持续尝试重连");
        }
        
        // 启动报警监控系统，每5秒检查一次报警信号
        AlarmMonitor::instance().start(5000);
        SPDLOG_INFO("已启动报警监控系统");

        // 等待关闭信号
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex);
            shutdown_cv.wait(lock, []() { return shutdown_requested; });
        }

        // 停止报警监控
        AlarmMonitor::instance().stop();
        SPDLOG_INFO("已停止报警监控系统");

        // 关闭服务器
        server.close().wait();
        SPDLOG_INFO("服务器已关闭");

    } catch (const std::exception& e) {
        SPDLOG_ERROR("服务器异常退出: {}", e.what());
        return 1;
    }

    return 0;
}