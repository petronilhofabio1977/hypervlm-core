#include "../include/workqueue.hpp"
#include <cassert>

namespace atm2 {

// Michael & Scott Lock-Free MPMC Queue

WorkQueue::WorkQueue() {
    Node* dummy = new Node(nullptr);
    head.store(dummy);
    tail.store(dummy);
}

WorkQueue::~WorkQueue() {
    Node* node = head.load();
    while (node) {
        Node* next = node->next.load();
        delete node;
        node = next;
    }
}

void WorkQueue::push(std::function<void()> task) {
    Node* node = new Node(std::move(task));
    node->next.store(nullptr);

    Node* t;
    Node* next;

    while (true) {
        t = tail.load();
        next = t->next.load();

        if (t == tail.load()) {
            if (next == nullptr) {
                if (t->next.compare_exchange_weak(next, node)) {
                    tail.compare_exchange_weak(t, node);
                    return;
                }
            } else {
                tail.compare_exchange_weak(t, next);
            }
        }
    }
}

bool WorkQueue::pop(std::function<void()> &task) {
    Node* h;
    Node* t;
    Node* next;

    while (true) {
        h = head.load();
        t = tail.load();
        next = h->next.load();

        if (h == head.load()) {
            if (h == t) {
                if (next == nullptr)
                    return false;
                tail.compare_exchange_weak(t, next);
            } else {
                task = next->value;
                if (head.compare_exchange_weak(h, next)) {
                    delete h;
                    return true;
                }
            }
        }
    }
}

} // namespace atm2
