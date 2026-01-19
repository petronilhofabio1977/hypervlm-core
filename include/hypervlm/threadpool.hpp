#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <future>
#include <mutex>

namespace hypervlm {

class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using R = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<R()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<R> res = task->get_future();
        {
            std::lock_guard lk(tasks_mtx);
            tasks.emplace([task](){ (*task)(); });
        }
        cv.notify_one();
        return res;
    }

    void submit_void(std::function<void()> fn);
    void shutdown();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex tasks_mtx;
    std::condition_variable cv;
    std::atomic<bool> stopping{false};

    void worker_loop();
};

} // namespace hypervlm
