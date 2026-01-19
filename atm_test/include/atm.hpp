#pragma once
#include <cstdint>
#include <functional>
#include <cstddef>

namespace atm {

struct SchedulerConfig {
    uint64_t n_virtual_nodes = 4096;
    size_t n_workers = 0;
    size_t max_queue_per_worker = 1 << 16;
    bool verbose = false;
};

class Scheduler {
public:
    explicit Scheduler(const SchedulerConfig &cfg = SchedulerConfig());
    ~Scheduler();

    void submit(std::function<void()> task, uint64_t virtual_node_id);
    void wait_all();
    size_t queue_length() const;
    size_t workers() const noexcept;

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

private:
    struct Impl;
    Impl *impl_;
};

} // namespace atm
