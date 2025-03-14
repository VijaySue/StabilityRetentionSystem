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
#include <csignal>

// 信号处理
std::condition_variable_any shutdown_cv;
std::mutex shutdown_mutex;
bool shutdown_requested = false;

void signal_handler(int signal) {
    SPDLOG_INFO("收到信号: {}, 准备关闭服务", signal);
    {
        std::lock_guard<std::mutex> lock(shutdown_mutex);
        shutdown_requested = true;
    }
    shutdown_cv.notify_all();
}

int main(int argc, char* argv[]) {
    // 配置日志格式
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

    try {
        // 注册信号处理
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // 构建服务器URI
        utility::string_t address = U("0.0.0.0");
        int port = 8080;

        // 支持命令行参数覆盖默认端口
        if (argc > 1) {
            try {
                port = std::stoi(argv[1]);
            }
            catch (const std::exception& e) {
                SPDLOG_WARN("命令行端口参数无效: {}, 使用默认端口", argv[1]);
            }
        }

        web::uri_builder uri;
        uri.set_scheme(U("http"))
           .set_host(address)
           .set_port(port);

        // 启动HTTP服务器
        StabilityServer server(uri.to_uri().to_string());
        server.open().wait();

        SPDLOG_INFO("稳定性保持系统服务已启动");
        SPDLOG_INFO("监听地址: {}", utility::conversions::to_utf8string(uri.to_uri().to_string()));
        SPDLOG_INFO("版本: {}", constants::VERSION);

        // 等待关闭信号
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex);
            shutdown_cv.wait(lock, [] { return shutdown_requested; });
        }

        // 关闭服务器
        SPDLOG_INFO("正在关闭服务...");
        server.close().wait();
        SPDLOG_INFO("服务已正常关闭");
    }
    catch (const std::exception& e) {
        SPDLOG_CRITICAL("服务启动失败: {}", e.what());
        return 1;
    }
    return 0;
}