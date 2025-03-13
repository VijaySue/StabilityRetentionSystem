// task_manager.h
#pragma once
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

/**
 * @brief 异步任务管理器（单例）
 * @note 处理文档中所有异步操作（带回调的接口）
 */
struct AsyncTask {
    int taskId;             // 文档中的taskId
    int defectId;           // 文档中的defectId
    std::string operation;  // 操作类型（"刚性支撑"/"平台升高等"）
    std::string target;     // 操作目标（如平台编号）
};

class TaskManager {
public:
    static TaskManager& instance();

    /**
     * @brief 创建异步任务
     * @param taskId 任务标识（用于回调匹配）
     * @param operation 操作指令（需符合文档定义）
     */
    void create_task(int taskId, int defectId,
        const std::string& operation,
        const std::string& target = "");

private:
    TaskManager();
    ~TaskManager();

    // 工作线程函数
    void worker_thread();

    // 任务队列相关
    std::queue<AsyncTask> m_tasks;        // 待处理任务队列
    std::mutex m_mutex;                   // 队列访问锁
    std::condition_variable m_cv;         // 条件变量
    std::thread m_worker;                 // 工作线程
    bool m_running = true;                // 线程运行标志
};