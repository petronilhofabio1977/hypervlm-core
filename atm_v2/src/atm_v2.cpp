#include "../include/atm_v2.hpp"
#include "config.hpp"
#include "workqueue.hpp"

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace atm2 {

struct ATMv2::Impl {
    ATMConfig cfg;

    uint64_t n_workers;
    uint64_t n_clusters;
    uint64_t n_vnodes;

    std::vector<std::thread> workers;
    std::vector<WorkQueue*> queues;

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> outstanding{0};

    std::mutex wait_mtx;
    std::condition_variable wait_cv;

    Impl(const ATMConfig &c)
        : cfg(c),
          n_workers(c.workers > 0 ? c.workers : std::thread::hardware_concurrency()),
          n_clusters(c.clusters),
          n_vnodes(c.virtual_nodes)
    {
        // Criar filas sem capacidade fixa (lock-free)
        queues.reserve(n_workers);
        for (size_t i = 0; i < n_workers; ++i)
            queues.push_back(new WorkQueue());

        // Criar threads
        for (size_t i = 0; i < n_workers; ++i)
            workers.emplace_back([this, i]() { worker_loop(i); });
    }

    ~Impl() {
        stop.store(true);

        // Empurrar no-op para acordar workers
        for (size_t i = 0; i < n_workers; ++i)
            queues[i]->push([](){});

        // Esperar threads
        for (auto &t : workers)
            if (t.joinable()) t.join();

        for (auto *q : queues)
            delete q;
    }

    inline size_t vnode_to_worker(uint64_t vnode) const {
        uint64_t cluster = vnode % n_clusters;
        return cluster % n_workers;
    }

    void submit_task(std::function<void()> task, uint64_t vnode) {
        outstanding++;

        size_t wid = vnode_to_worker(vnode);
        queues[wid]->push(std::move(task));
    }

    void worker_loop(size_t wid) {
        std::function<void()> task;

        while (!stop.load()) {
            if (queues[wid]->pop(task)) {
                if (task) {
                    task();
                    if (outstanding.fetch_sub(1) == 1) {
                        std::lock_guard lg(wait_mtx);
                        wait_cv.notify_all();
                    }
                }
            } else {
                // Work-stealing simples
                size_t victim = (wid + 1) % n_workers;
                if (queues[victim]->pop(task)) {
                    if (task) {
                        task();
                        if (outstanding.fetch_sub(1) == 1) {
                            std::lock_guard lg(wait_mtx);
                            wait_cv.notify_all();
                        }
                    }
                } else {
                    std::this_thread::yield();
                }
            }
        }
    }

    void wait_all_tasks() {
        std::unique_lock lk(wait_mtx);
        wait_cv.wait(lk, [&] {
            return outstanding.load() == 0;
        });
    }

    size_t queue_size() const noexcept {
        return 0; // Não usamos mais size() com MPSC queue
    }
};

ATMv2::ATMv2(const ATMConfig &cfg) {
    impl_ = new Impl(cfg);
}

ATMv2::~ATMv2() {
    delete impl_;
}

void ATMv2::submit(std::function<void()> task, uint64_t vnode) {
    impl_->submit_task(std::move(task), vnode);
}

void ATMv2::wait_all() {
    impl_->wait_all_tasks();
}

size_t ATMv2::queue_length() const noexcept {
    return impl_->queue_size();
}

size_t ATMv2::workers() const noexcept {
    return impl_->n_workers;
}

} // namespace atm2
