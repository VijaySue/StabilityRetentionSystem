/**
 * @file main.cpp
 * @brief 稳定性保持系统主程序入口
 * @author VijaySue
 * @date 2024-3-11
 */

// main.cpp
#include <spdlog/spdlog.h>
#include <cpprest/uri_builder.h>
#include "../include/server.h"
#include "../include/common.h"
#include "../include/config_manager.h"
#include <csignal>

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
    // 加载配置文件
    if (!ConfigManager::instance().load_config("config/config.ini")) {
        std::cerr << "无法加载配置文件，使用默认配置" << std::endl;
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

        // 等待关闭信号
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex);
            shutdown_cv.wait(lock, []() { return shutdown_requested; });
        }

        // 关闭服务器
        server.close().wait();
        SPDLOG_INFO("服务器已关闭");

    } catch (const std::exception& e) {
        SPDLOG_ERROR("服务器异常退出: {}", e.what());
        return 1;
    }

    return 0;
}