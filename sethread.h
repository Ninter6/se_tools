/*
 * @Author: Ninter6 mc525740@outlook.com
 * @Date: 2024-01-06 17:52:23
 * @LastEditors: Ninter6
 * @LastEditTime: 2024-01-20 22:27:30
 */
#pragma once

#include <future>
#include <mutex>
#include <atomic>
#include <queue>
#include <iostream>

namespace st {
    template <class Func_T, class = void>
    class Service;

    template<class Func_T>
    class Worker {
    public:
        Worker(Service<Func_T>* service) : m_Service(service) {
            m_Thread = std::thread([&]{
                run();
            });
        }

        Worker(const Worker&) = delete;
        Worker(Worker&&) = default;
        Worker& operator=(const Worker&) = delete;
        Worker& operator=(Worker&&) = default;

        ~Worker() {
            stop();
        }

        void run() {
            while (!m_ShouldStop && !m_Service->hasShutDown()) {
                auto task = m_Service->getTask();
                if (task.valid()) {
                    task();
                    m_Service->workDone(task.get_future());
                } else {
                    std::this_thread::yield();
                }
            }
        }

        void stop() {
            m_ShouldStop = true;
            m_Thread.join();
        }

        Service<Func_T>* m_Service;
        std::thread m_Thread;

        bool m_ShouldStop{false};
    };

    template <class Func_T, class>
    class Service {
    public:
        Service(std::function<Func_T> func, size_t num_worker) 
        : m_Func(func) {
            m_Workers.reserve(num_worker);
            for (int i = 0; i < num_worker; i++) m_Workers.emplace_back(this);
        }

        Service(const Service&) = delete;
        Service& operator=(const Service&) = delete;

        ~Service() {
            waitForCompletion();
            shutDown();
            m_Workers.clear();
        }

        template <class...Args>
        std::enable_if_t<std::is_invocable_v<Func_T, Args...>>
        addTask(Args...args) {
            m_RemainingTasks++;
            std::lock_guard lock{m_Mutex};
            m_Tasks.emplace([args..., this]{m_Func(args...);});
        }

        std::packaged_task<void()> getTask() {
            if (m_Tasks.empty()) {
                return {};
            } else {
                std::lock_guard lock{m_Mutex};
                auto task = std::move(m_Tasks.front());
                m_Tasks.pop();
                return std::packaged_task(task);
            }
        }

        static void wait() {
            std::this_thread::yield();
        }

        void waitForCompletion() const {
            while (!allTasksCompleted()) wait();
        }

        bool allTasksCompleted() const {
            return m_RemainingTasks == 0;
        }

        void shutDown() {
            m_HasShutDown = true;
        }

        bool hasShutDown() const {
            return m_HasShutDown;
        }

    private:
        std::atomic<size_t> m_RemainingTasks = 0;
        std::atomic_bool m_HasShutDown = false;

        std::mutex m_Mutex;

        std::function<Func_T> m_Func;
        std::queue<std::function<void()>> m_Tasks;
        std::vector<Worker<Func_T>> m_Workers;

        void workDone(std::future<void>&& res) {
            m_RemainingTasks--;
        }

        friend Worker<Func_T>;
    };

    template <class R_T, class...Args>
    class Service<R_T(Args...), std::enable_if_t<!std::is_same_v<R_T, void>>> {
    public:
        Service(std::function<R_T(Args...)> func, size_t num_worker) 
        : m_Func(func) {
            m_Workers.reserve(num_worker);
            for (int i = 0; i < num_worker; i++) m_Workers.emplace_back(this);
        }

        ~Service() {
            shutDown();
            m_Workers.clear();
        }

        Service(const Service&) = delete;
        Service& operator=(const Service&) = delete;

        template <class..._Args>
        std::enable_if_t<std::is_invocable_v<R_T(Args...), _Args...>>
        addTask(_Args...args) {
            m_RemainingTasks++;
            std::lock_guard lock{m_Mutex};
            m_Tasks.emplace([args..., this]{return m_Func(args...);});
        }

        std::packaged_task<R_T()> getTask() {
            if (m_Tasks.empty()) {
                return {};
            } else {
                std::lock_guard lock{m_Mutex};
                auto task = std::move(m_Tasks.front());
                m_Tasks.pop();
                return task;
            }
        }

        R_T getResult() {
            while (m_Results.empty()) wait();
            std::lock_guard lock{m_Mutex};
            auto res = std::move(m_Results.front());
            m_Results.pop();
            return res.get();
        }

        static void wait() {
            std::this_thread::yield();
        }

        void waitForCompletion() const {
            while (!allTasksCompleted()) wait();
        }

        bool allTasksCompleted() const {
            return m_RemainingTasks == 0;
        }

        void workDone(std::future<R_T>&& res) {
            m_RemainingTasks--;
            std::lock_guard lock{m_Mutex};
            m_Results.emplace(std::move(res));
        }

        void shutDown() {
            m_HasShutDown = true;
        }

        bool hasShutDown() const {
            return m_HasShutDown;
        }

    private:
        std::atomic<size_t> m_RemainingTasks = 0;
        std::atomic_bool m_HasShutDown = false;

        std::mutex m_Mutex;

        std::function<R_T(Args...)> m_Func;
        std::queue<std::packaged_task<R_T()>> m_Tasks;
        std::queue<std::future<R_T>> m_Results;
        std::vector<Worker<R_T(Args...)>> m_Workers;
    };

    class ThreadPool {

    };
}