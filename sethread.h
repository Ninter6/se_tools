/*
 * @Author: Ninter6 mc525740@outlook.com
 * @Date: 2024-01-06 17:52:23
 * @LastEditors: Ninter6
 * @LastEditTime: 2024-02-10 23:42:22
 */
#pragma once

#include <future>
#include <mutex>
#include <atomic>
#include <queue>
#include <unordered_map>
#include <cassert>

// helper macros
#define NONE_COPY(c) \
    c(const c&) = delete; \
    c& operator=(const c&) = delete;
#define NONE_MOVE(c) \
    c(c&&) = delete; \
    c& operator=(c&&) = delete;
#define DEFAULT_COPY(c) \
    c(const c&) = default; \
    c& operator=(const c&) = default;
#define DEFAULT_MOVE(c) \
    c(c&&) = default; \
    c& operator=(c&&) = default;

namespace st {

struct ID {
    ID() = default;
    ID(size_t id) : m_Id(id) {}
    ID(std::string_view name) : m_Id(std::hash<std::string_view>{}(name)) {}

    bool operator<(const ID& other) const {
        return m_Id < other.m_Id;
    }
    bool operator>(const ID& other) const {
        return m_Id > other.m_Id;
    }
    bool operator==(const ID& other) const {
        return m_Id == other.m_Id;
    }

    size_t get() const {
        return m_Id;
    }

private:
    size_t m_Id;
};

}

template <>
struct std::hash<st::ID> {
    size_t operator()(const st::ID& id) const {
        return id.get();
    }
};

namespace st {

template <class>
struct result_of{};
template <class F, class...Args>
struct result_of<F(Args...)> {
    using type = F;
};

template <class _Service, class _Res>
class Worker {
public:
    Worker(_Service* service)
    : m_Service(service), m_Thread([&]{run();}) {}

    NONE_COPY(Worker)
    DEFAULT_MOVE(Worker)

    ~Worker() {
        stop();
    }

    void run() {
        while (!m_ShouldStop && !m_Service->hasShutDown()) {
            std::function<_Res()> func;
            m_Service->getTask(func);
            if (func != nullptr) {
                std::packaged_task<_Res()> task{func};
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

private:
    _Service* m_Service;
    std::thread m_Thread;

    bool m_ShouldStop{false};
};

class BaseService {
public:
    BaseService() = default;
    virtual ~BaseService() = default;

    ID getID() const {
        return m_ID;
    }

    virtual void shutDown() = 0;

    virtual bool hasShutDown() const = 0;

    virtual bool allTasksCompleted() const = 0;

    virtual bool allResultsGot() const = 0;

    virtual void waitForCompletion() const = 0;

    static void wait() {
        std::this_thread::yield();
    }

private:
    ID m_ID;

    friend class ThreadPool;
    
};

template <class Func_T, class = std::enable_if_t<std::is_function_v<Func_T>>>
class Service : public BaseService {
public:
    using Type = Service<Func_T, void>;
    using Res = result_of<Func_T>::type;
    using Worker_t = Worker<Type, Res>;

    template <class _T, class = std::is_constructible<std::function<Func_T>, _T>>
    Service(const _T& func, size_t num_worker) : m_Func(func) {
        m_Workers.reserve(num_worker);
        for (int i = 0; i < num_worker; i++) m_Workers.emplace_back(this);
    }

    ~Service() {
        waitForCompletion();
        shutDown();
        m_Workers.clear();
    }

    NONE_COPY(Service);
    DEFAULT_MOVE(Service);

    template <class..._Args>
    std::enable_if_t<std::is_invocable_v<Func_T, _Args...>>
    addTask(_Args&&...args) {
        m_RemainingTasks++;
        std::lock_guard lock{m_Mutex};
        // copy arguments
        m_Tasks.emplace([args..., this]{return m_Func(args...);});
    }

    void getTask(std::function<Res()>& task) {
        if (m_Tasks.empty()) {
            task = nullptr;
        } else {
            std::lock_guard lock{m_Mutex};
            task = std::move(m_Tasks.front());
            m_Tasks.pop();
        }
    }

    Res getResult() {
        while (m_Results.empty()) wait();
        std::lock_guard lock{m_Mutex};
        auto result = m_Results.front().get();
        m_Results.pop();
        return result;
    }

    virtual void shutDown() override {
        m_HasShutDown = true;
    }

    virtual bool hasShutDown() const override {
        return m_HasShutDown;
    }

    virtual bool allTasksCompleted() const override {
        return m_RemainingTasks == 0;
    }

    virtual void waitForCompletion() const override {
        while (!m_HasShutDown && !allTasksCompleted()) wait();
    }

    bool allResultsGot() const {
        return allTasksCompleted() && m_Results.size() == 0;
    }

private:
    std::function<Func_T> m_Func;
    std::queue<std::function<Res()>> m_Tasks;
    std::queue<std::future<Res>> m_Results;
    std::vector<Worker_t> m_Workers;

    std::mutex m_Mutex;

    std::atomic<size_t> m_RemainingTasks = 0;
    std::atomic_bool m_HasShutDown = false;

    void workDone(std::future<Res>&& res) {
        {
            std::lock_guard lock{m_Mutex};
            m_Results.emplace(std::move(res));
        }
        m_RemainingTasks--;
    }

    friend Worker_t;
    
};

template <class...Args>
class Service<void(Args...)> : public BaseService {
public:
    using Type = Service<void(Args...), void>;
    using Res = void;
    using Worker_t = Worker<Type, Res>;

    template <class _T, class = std::is_constructible<std::function<void(Args...)>, _T>>
    Service(const _T& func, size_t num_worker) : m_Func(func) {
        m_Workers.reserve(num_worker);
        for (int i = 0; i < num_worker; i++) m_Workers.emplace_back(this);
    }

    ~Service() {
        waitForCompletion();
        shutDown();
        m_Workers.clear();
    }

    NONE_COPY(Service);
    DEFAULT_MOVE(Service);

    template <class..._Args>
    std::enable_if_t<std::is_invocable_v<void(Args...), _Args...>>
    addTask(_Args&&...args) {
        m_RemainingTasks++;
        std::lock_guard lock{m_Mutex};
        // copy arguments
        m_Tasks.emplace([args..., this]{m_Func(args...);});
    }

    void getTask(std::function<Res()>& task) {
        if (m_Tasks.empty()) {
            task = nullptr;
        } else {
            std::lock_guard lock{m_Mutex};
            task = std::move(m_Tasks.front());
            m_Tasks.pop();
        }
    }

    void getResult() {
        assert(false && "none result expected");
    }

    virtual void shutDown() override {
        m_HasShutDown = true;
    }

    virtual bool hasShutDown() const override {
        return m_HasShutDown;
    }

    virtual bool allTasksCompleted() const override {
        return m_RemainingTasks == 0;
    }

    virtual void waitForCompletion() const override {
        while (!m_HasShutDown && !allTasksCompleted()) wait();
    }

private:
    std::function<void(Args...)> m_Func;
    std::queue<std::function<Res()>> m_Tasks;
    std::vector<Worker_t> m_Workers;

    std::mutex m_Mutex;

    std::atomic<size_t> m_RemainingTasks = 0;
    std::atomic_bool m_HasShutDown = false;

    void workDone(std::future<Res>&& res) {
        m_RemainingTasks--;
    }

    friend Worker_t;
    
};

class ThreadPool {
public:
    ThreadPool() = default;
    NONE_COPY(ThreadPool);

    template <class Func_T, class = std::enable_if_t<std::is_function_v<Func_T>>>
    ThreadPool& addService(ID id, auto&& func, size_t num_workers) {
        const auto& [success, it] = m_Services.emplace(id, std::make_unique<Service<Func_T>>(func, num_workers));
        if (success) it->ID = id;
        return *this;
    }

    template <class Func_T>
    Service<Func_T>& getService(ID id) {
        auto it = m_Services.find(id);
        if (it == m_Services.end()) {
            throw std::runtime_error("service not found");
        }
        return *static_cast<Service<Func_T>&>(*(it->second));
    }

    bool allTasksCompleted(ID id) const {
        return m_Services.at(id)->allTasksCompleted();
    }

    void waitForCompletion(ID id) const {
        m_Services.at(id)->waitForCompletion();
    }

private:
    std::unordered_map<ID, std::unique_ptr<BaseService>> m_Services;
};

}