// main.cpp
#include <spdlog/spdlog.h>
#include "server.h"
#include "common.h"

int main() {
    // 配置日志格式
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");

    try {
        // 启动HTTP服务器
        utility::string_t url = U("http://0.0.0.0:8080");
        StabilityServer server(url);
        server.open().wait();

        SPDLOG_INFO("稳定性保持系统服务已启动，监听地址: {}", url);
        SPDLOG_INFO("版本: {}", constants::VERSION);

        // 保持运行直到输入回车
        std::cin.get();

        // 关闭服务器
        server.close().wait();
        SPDLOG_INFO("服务已正常关闭");
    }
    catch (const std::exception& e) {
        SPDLOG_CRITICAL("服务启动失败: {}", e.what());
        return 1;
    }
    return 0;
}