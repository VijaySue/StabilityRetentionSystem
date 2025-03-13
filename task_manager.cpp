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
    m_running = false;
    m_cv.notify_all();
    if (m_worker.joinable()) {
        m_worker.join();
    }
    SPDLOG_INFO("任务管理器关闭");
}

void TaskManager::create_task(int taskId, int defectId,
    const std::string& operation,
    const std::string& target) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.push({ taskId, defectId, operation, target });
    m_cv.notify_one();
    SPDLOG_DEBUG("创建任务: {}[{}] {}", taskId, target, operation);
}

void TaskManager::worker_thread() {
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_tasks.empty() || !m_running; });

        if (!m_tasks.empty()) {
            AsyncTask task = m_tasks.front();
            m_tasks.pop();
            lock.unlock();

            try {
                // 执行PLC操作
                PLCManager::instance().execute_operation(task.operation);
                SPDLOG_INFO("任务完成，ID: {}，操作: {}", task.taskId, task.operation);

                // 构造回调状态信息
                std::string state = "已" + task.operation;
                if (!task.target.empty()) {
                    state += "（平台" + task.target + "）";
                }

                // 发送回调
                CallbackClient::instance().send_callback(
                    task.taskId,
                    task.defectId,
                    state
                );
            }
            catch (const std::exception& e) {
                SPDLOG_ERROR("任务执行失败，ID: {}，错误: {}", task.taskId, e.what());
            }
        }
    }
}