// task_manager.cpp
#include "task_manager.h"
#include "plc_manager.h"
#include "callback_client.h"
#include <spdlog/spdlog.h>

TaskManager& TaskManager::instance() {
    static TaskManager instance;
    return instance;
}

TaskManager::TaskManager() {
    m_worker = std::thread(&TaskManager::worker_thread, this);
    SPDLOG_INFO("任务管理器启动");
}

TaskManager::~TaskManager() {
    m_running = false;         // 设置停止标志
    m_cv.notify_all();         // 唤醒等待线程
    if (m_worker.joinable()) {
        m_worker.join();       // 等待线程退出
    }
    SPDLOG_INFO("任务管理器关闭");
}

void TaskManager::create_task(int taskId, int defectId,
    const std::string& operation,
    const std::string& target) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.push({ taskId, defectId, operation, target });
    m_cv.notify_one();  // 通知工作线程有新任务
    SPDLOG_DEBUG("创建任务: {}[{}] {}", taskId, target, operation);
}

void TaskManager::worker_thread() {
    while (m_running) {  // 主循环直到停止标志置位
        std::unique_lock<std::mutex> lock(m_mutex);
        // 等待条件：有新任务或需要停止
        m_cv.wait(lock, [this] { return !m_tasks.empty() || !m_running; });

        if (!m_tasks.empty()) {
            // 获取队列首任务（临界区操作）
            AsyncTask task = m_tasks.front();
            m_tasks.pop();
            lock.unlock();  // 提前释放锁，允许其他线程操作队列

            try {
                // 执行PLC设备操作（可能阻塞）
                PLCManager::instance().execute_operation(task.operation);
                SPDLOG_INFO("任务完成，ID: {}，操作: {}", task.taskId, task.operation);

                /* 构造回调状态文本：
                 * - 基础格式："已[操作]"
                 * - 带目标平台示例："已升高（平台1）"
                 */
                std::string state = "已" + task.operation;
                if (!task.target.empty()) {
                    state += "（平台" + task.target + "）";
                }

                // 发送回调通知边缘系统
                CallbackClient::instance().send_callback(
                    task.taskId,
                    task.defectId,
                    state
                );
            }
            catch (const std::exception& e) {
                // 捕获所有异常并记录错误（保证线程不崩溃）
                SPDLOG_ERROR("任务执行失败，ID: {}，错误: {}", task.taskId, e.what());
            }
        }
    }
}