/**
 * @file task_manager.cpp
 * @brief 异步任务管理器实现
 * @details 负责处理异步任务队列，执行设备控制操作，并发送回调信息
 * @author VijaySue
 * @version 2.0
 * @date 2024-3-29
 */
#include "../include/task_manager.h"
#include "../include/plc_manager.h"
#include "../include/callback_client.h"
#include <spdlog/spdlog.h>
#include <cpprest/http_client.h>

/**
 * @brief 获取TaskManager单例实例
 * @return TaskManager单例的引用
 */
TaskManager& TaskManager::instance() {
    static TaskManager instance;
    return instance;
}

/**
 * @brief 构造函数 - 初始化任务管理器
 */
TaskManager::TaskManager() {
    m_worker = std::thread(&TaskManager::worker_thread, this);
    SPDLOG_INFO("任务管理器启动");
}

/**
 * @brief 析构函数 - 安全关闭任务管理器
 */
TaskManager::~TaskManager() {
    m_running = false;         // 设置停止标志
    m_cv.notify_all();         // 唤醒等待线程
    if (m_worker.joinable()) {
        m_worker.join();       // 等待线程退出
    }
    SPDLOG_INFO("任务管理器关闭");
}

/**
 * @brief 创建异步任务
 * @param taskId 任务ID
 * @param defectId 缺陷ID
 * @param operation 操作指令
 * @param target 操作目标（可选）
 */
void TaskManager::create_task(int taskId, int defectId,
    const std::string& operation,
    const std::string& target) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.push({ taskId, defectId, operation, target });
    m_cv.notify_one();  // 通知工作线程有新任务
    SPDLOG_INFO("创建任务: ID={}, 缺陷ID={}, 操作={}, 目标={}", 
                taskId, defectId, operation, target.empty() ? "无" : target);
}

/**
 * @brief 工作线程函数
 * @details 从任务队列中获取任务并执行
 */
void TaskManager::worker_thread() {
    while (m_running) {  // 主循环直到停止标志置位
        std::unique_lock<std::mutex> lock(m_mutex);
        // 等待条件：有新任务或需要停止
        m_cv.wait(lock, [this] { return !m_tasks.empty() || !m_running; });

        if (!m_tasks.empty() && m_running) {
            // 获取队列首任务（临界区操作）
            AsyncTask task = m_tasks.front();
            m_tasks.pop();
            lock.unlock();  // 提前释放锁，允许其他线程操作队列

            try {
                SPDLOG_INFO("开始执行任务: ID={}, 操作={}", task.taskId, task.operation);
                
                // 映射操作名称到PLC操作指令
                std::string plc_command;
                
                // 映射关系: JSON.state → task.operation, JSON.platformNum → task.target
                // 根据API请求的state(operation)和platformNum(target)映射到PLC操作命令
                if (task.operation == "刚性支撑") {  // JSON请求中state="刚性支撑"
                    // 对应M22.1
                    plc_command = "刚性支撑";
                }
                else if (task.operation == "柔性复位") {  // JSON请求中state="柔性复位"
                    // 对应M22.2
                    plc_command = "柔性复位";
                }
                else if (task.operation == "升高") {  // JSON请求中state="升高"
                    if (task.target == "1") {  // JSON请求中platformNum=1
                        // 对应M22.3
                        plc_command = "平台1上升";
                    } else if (task.target == "2") {  // JSON请求中platformNum=2
                        // 对应M22.5
                        plc_command = "平台2上升";
                    } else {
                        // 默认为平台1
                        plc_command = "平台1上升";
                    }
                }
                else if (task.operation == "复位") {  // JSON请求中state="复位"
                    if (task.target == "1") {  // JSON请求中platformNum=1
                        // 对应M22.4
                        plc_command = "平台1复位";
                    } else if (task.target == "2") {  // JSON请求中platformNum=2
                        // 对应M22.6
                        plc_command = "平台2复位";
                    } else {
                        // 默认为平台1
                        plc_command = "平台1复位";
                    }
                }
                else if (task.operation == "调平") {  // JSON请求中state="调平"
                    if (task.target == "1") {  // JSON请求中platformNum=1
                        // 对应M22.7
                        plc_command = "平台1调平";
                    } else if (task.target == "2") {  // JSON请求中platformNum=2
                        // 对应M23.1
                        plc_command = "平台2调平";
                    } else {
                        // 默认为平台1
                        plc_command = "平台1调平";
                    }
                }
                else if (task.operation == "调平复位") {  // JSON请求中state="调平复位"
                    if (task.target == "1") {  // JSON请求中platformNum=1
                        // 对应M23.0
                        plc_command = "平台1调平复位";
                    } else if (task.target == "2") {  // JSON请求中platformNum=2
                        // 对应M23.2
                        plc_command = "平台2调平复位";
                    } else {
                        // 默认为平台1
                        plc_command = "平台1调平复位";
                    }
                }
                else {
                    // 未知操作类型，直接传递给PLC
                    plc_command = task.operation;
                    SPDLOG_WARN("未识别的操作类型: {}, 将直接传递给PLC", task.operation);
                }
                
                // 执行PLC设备操作
                bool operation_success = PLCManager::instance().execute_operation(plc_command);
                
                if (operation_success) {
                    SPDLOG_INFO("任务执行成功，ID: {}，操作: {}", task.taskId, plc_command);
                    
                    // 构造回调状态文本
                    std::string state = "success";
                    
                    // 根据操作类型选择不同的回调方法
                    if (task.operation == "刚性支撑" || task.operation == "柔性复位") {
                        // 支撑控制回调
                        std::string callbackState = (task.operation == "刚性支撑") ? "已刚性支撑" : "已柔性复位";
                        CallbackClient::instance().send_support_callback(
                            task.taskId,
                            task.defectId,
                            callbackState
                        );
                    }
                    else if (task.operation == "升高" || task.operation == "复位") {
                        // 平台高度回调
                        std::string callbackState = (task.operation == "升高") ? "已升高" : "已复位";
                        int platformNum = !task.target.empty() ? std::stoi(task.target) : 1;
                        CallbackClient::instance().send_platform_height_callback(
                            task.taskId,
                            task.defectId,
                            platformNum,
                            callbackState
                        );
                    }
                    else if (task.operation == "调平" || task.operation == "调平复位") {
                        // 平台调平回调
                        std::string callbackState = (task.operation == "调平") ? "已调平" : "已调平复位";
                        int platformNum = !task.target.empty() ? std::stoi(task.target) : 1;
                        CallbackClient::instance().send_platform_horizontal_callback(
                            task.taskId,
                            task.defectId,
                            platformNum,
                            callbackState
                        );
                    }
                    else {
                        SPDLOG_WARN("未知操作类型: {}, 无法发送回调", task.operation);
                    }
                } else {
                    // PLC操作失败，发送失败回调
                    SPDLOG_ERROR("任务执行失败，ID: {}，操作: {}", task.taskId, plc_command);
                    
                    std::string error_message = "PLC操作失败: " + plc_command;
                    
                    // 根据操作类型选择不同的回调方法
                    if (task.operation == "刚性支撑" || task.operation == "柔性复位") {
                        CallbackClient::instance().send_support_callback(
                            task.taskId,
                            task.defectId,
                            "error: " + error_message
                        );
                    }
                    else if (task.operation == "升高" || task.operation == "复位") {
                        int platformNum = !task.target.empty() ? std::stoi(task.target) : 1;
                        CallbackClient::instance().send_platform_height_callback(
                            task.taskId,
                            task.defectId,
                            platformNum,
                            "error: " + error_message
                        );
                    }
                    else if (task.operation == "调平" || task.operation == "调平复位") {
                        int platformNum = !task.target.empty() ? std::stoi(task.target) : 1;
                        CallbackClient::instance().send_platform_horizontal_callback(
                            task.taskId,
                            task.defectId,
                            platformNum,
                            "error: " + error_message
                        );
                    }
                    else {
                        SPDLOG_WARN("未知操作类型: {}, 无法发送失败回调", task.operation);
                    }
                }
            }
            catch (const std::exception& e) {
                // 捕获所有异常并记录错误（保证线程不崩溃）
                SPDLOG_ERROR("任务执行失败，ID: {}，错误: {}", task.taskId, e.what());
                
                // 发送失败回调
                try {
                    std::string error_state = "error: " + std::string(e.what());
                    
                    // 尝试发送错误回调
                    if (task.operation == "刚性支撑" || task.operation == "柔性复位") {
                        CallbackClient::instance().send_support_callback(
                            task.taskId, task.defectId, error_state);
                    }
                    else if (task.operation == "升高" || task.operation == "复位") {
                        int platformNum = !task.target.empty() ? std::stoi(task.target) : 1;
                        CallbackClient::instance().send_platform_height_callback(
                            task.taskId, task.defectId, platformNum, error_state);
                    }
                    else if (task.operation == "调平" || task.operation == "调平复位") {
                        int platformNum = !task.target.empty() ? std::stoi(task.target) : 1;
                        CallbackClient::instance().send_platform_horizontal_callback(
                            task.taskId, task.defectId, platformNum, error_state);
                    }
                }
                catch (...) {
                    SPDLOG_ERROR("发送错误回调时发生异常");
                }
            }
        }
    }
}
