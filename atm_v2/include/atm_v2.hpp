#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include "config.hpp"

namespace atm2 {

class ATMv2 {
public:
    explicit ATMv2(const ATMConfig &cfg = ATMConfig());
    ~ATMv2();

    void submit(std::function<void()> task, uint64_t vnode);

    void wait_all();

    size_t queue_length() const noexcept;
    size_t workers() const noexcept;

private:
    struct Impl;
    Impl *impl_;
};

} // namespace atm2
