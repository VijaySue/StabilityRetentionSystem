/**
 * @file task_manager.h
 * @brief 异步任务管理器类定义
 * @details 管理异步任务的创建、执行和状态跟踪
 * @author VijaySue
 * @date 2024-3-11
 */
#pragma once
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

/**
 * @brief 异步任务结构体
 * @details 存储API请求中的任务信息，用于异步处理
 * @note JSON请求参数映射关系:
 *       - taskId → AsyncTask.taskId (任务ID)
 *       - defectId → AsyncTask.defectId (缺陷ID)
 *       - state → AsyncTask.operation (操作类型，如"升高"、"复位")
 *       - platformNum → AsyncTask.target (平台编号，转为字符串)
 */
struct AsyncTask {
    int taskId;             // 对应JSON请求中的taskId
    int defectId;           // 对应JSON请求中的defectId
    std::string operation;  // 对应JSON请求中的state (操作类型)
    std::string target;     // 对应JSON请求中的platformNum (操作目标)
};

class TaskManager {
public:
    static TaskManager& instance();

    /**
     * @brief 创建异步任务
     * @details 将API请求参数转换为异步任务并加入队列
     * @param taskId 任务标识（对应JSON中的taskId）
     * @param defectId 缺陷ID（对应JSON中的defectId）
     * @param operation 操作指令（对应JSON中的state，如"升高"、"复位"）
     * @param target 操作目标（对应JSON中的platformNum，如"1"、"2"）
     * @note JSON请求映射关系:
     *       - {"state": "升高", "platformNum": 1} →
     *         create_task(..., "升高", "1")
     */
    void create_task(int taskId, int defectId,
        const std::string& operation,  // 对应JSON中的state
        const std::string& target = ""); // 对应JSON中的platformNum

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