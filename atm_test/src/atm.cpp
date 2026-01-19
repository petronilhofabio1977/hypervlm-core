#include "atm.hpp"
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <memory>
#include <cassert>
#include <chrono>

namespace atm {

struct Scheduler::Impl {
    SchedulerConfig cfg;

    size_t n_workers;
    std::vector<std::thread> workers;

    struct WorkerQueue {
        std::deque<std::function<void()>> q;
        std::mutex mtx;
        std::condition_variable cv;
        std::atomic<size_t> qsize{0};
    };

    std::vector<std::unique_ptr<WorkerQueue>> wq;

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> outstanding_tasks{0};

    std::mutex wait_mtx;
    std::condition_variable wait_cv;

    Impl(const SchedulerConfig &c): cfg(c) {
        n_workers = (cfg.n_workers == 0 ? std::max<size_t>(1, std::thread::hardware_concurrency()) : cfg.n_workers);
        wq.reserve(n_workers);
        for (size_t i=0;i<n_workers;++i)
            wq.emplace_back(std::make_unique<WorkerQueue>());

        for (size_t i=0;i<n_workers;++i)
            workers.emplace_back([this, i](){ worker_loop(i); });
    }

    ~Impl() {
        stop.store(true);
        for (auto &w : wq)
            w->cv.notify_all();
        for (auto &t : workers)
            if (t.joinable()) t.join();
    }

    void worker_loop(size_t idx) {
        WorkerQueue &W = *wq[idx];
        while (!stop.load()) {
            std::function<void()> task;
            {
                std::unique_lock lk(W.mtx);
                W.cv.wait(lk, [&]{ return stop.load() || !W.q.empty(); });
                if (stop.load() && W.q.empty()) return;

                task = std::move(W.q.front());
                W.q.pop_front();
                W.qsize--;
            }

            try { task(); } catch(...) {}

            auto prev = outstanding_tasks.fetch_sub(1);
            if (prev == 1) {
                std::lock_guard lg(wait_mtx);
                wait_cv.notify_all();
            }
        }
    }

    inline size_t vnode_to_worker(uint64_t vnode) const noexcept {
        return vnode % n_workers;
    }

    void submit_task(std::function<void()> task, uint64_t vnode) {
        outstanding_tasks++;

        size_t widx = vnode_to_worker(vnode);
        WorkerQueue &W = *wq[widx];

        {
            std::lock_guard lk(W.mtx);
            W.q.emplace_back(std::move(task));
            W.qsize++;
        }
        W.cv.notify_one();
    }

    size_t total_queue_len() const {
        size_t s = 0;
        for (auto &w : wq) s += w->qsize;
        return s;
    }

    void wait_all_tasks() {
        std::unique_lock lk(wait_mtx);
        wait_cv.wait(lk, [&]{ return outstanding_tasks.load() == 0; });
    }
};

Scheduler::Scheduler(const SchedulerConfig &cfg) {
    impl_ = new Impl(cfg);
}
Scheduler::~Scheduler() { delete impl_; }
void Scheduler::submit(std::function<void()> task, uint64_t vnode) { impl_->submit_task(std::move(task), vnode); }
void Scheduler::wait_all() { impl_->wait_all_tasks(); }
size_t Scheduler::queue_length() const { return impl_->total_queue_len(); }
size_t Scheduler::workers() const noexcept { return impl_->n_workers; }

} // namespace atm
