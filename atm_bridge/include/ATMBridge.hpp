#pragma once
#include <functional>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ATMBridge{
public:
    using Task=std::function<void()>;
    ATMBridge(size_t n=4);
    ~ATMBridge();
    void submit(Task t);
    void shutdown();
private:
    std::vector<std::thread> workers;
    std::queue<Task> q;
    std::mutex m;
    std::condition_variable cv;
    std::atomic<bool> run{true};
};
