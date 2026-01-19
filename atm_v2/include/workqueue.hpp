#pragma once
#include <atomic>
#include <functional>

namespace atm2 {

class WorkQueue {
public:
    WorkQueue();
    ~WorkQueue();

    void push(std::function<void()> task);
    bool pop(std::function<void()> &task);

private:
    struct Node {
        std::function<void()> value;
        std::atomic<Node*> next;
        Node(std::function<void()> v) : value(std::move(v)), next(nullptr) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;
};

} // namespace atm2
