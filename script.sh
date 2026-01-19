#!/usr/bin/env bash
set -euo pipefail

ROOT="$(pwd)"
TS=$(date +%Y%m%d_%H%M%S)
BACKUP="backup_${TS}"

echo ">>> Backup em ${BACKUP}"
mkdir -p "$BACKUP"
rsync -a --delete src/ "$BACKUP/src/"
rsync -a --delete include/ "$BACKUP/include/" 2>/dev/null || true

# ============================
# 1) microop.cpp SEM AVX
# ============================
mkdir -p src/dna/src
cat > src/dna/src/microop.cpp << 'EOM'
#include <dna/microop.hpp>
#include <cstdint>
#include <cstring>
#include <vector>

static inline uint64_t popcount64(uint64_t v) {
    return (uint64_t)__builtin_popcountll(v);
}

namespace dna {

void MicroOpExecutor::execute_batch(const std::vector<MicroOp> &ops) {

    for(const auto &op : ops) {

        switch(op.id) {

        case MicroOpId::POPCOUNT: {
            const uint64_t *data = (const uint64_t*)op.arg0;
            size_t n = (size_t)op.arg1;
            uint64_t total = 0;

            for(size_t i=0; i<n; i++)
                total += popcount64(data[i]);

            (void)total;
            break;
        }

        case MicroOpId::MEMCOPY: {
            void *dst       = (void*)op.arg0;
            const void *src = (const void*)op.arg1;
            size_t n        = (size_t)op.arg2;
            memcpy(dst, src, n);
            break;
        }

        default:
            break;
        }
    }
}

} // namespace dna
EOM

# ============================
# 2) OOO MODULE HEADER
# ============================
mkdir -p src/dna/include/dna
cat > src/dna/include/dna/ooo.hpp << 'EOM'
#pragma once
#include <vector>
#include <optional>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <thread>
#include <unordered_map>
#include "dna/microop.hpp"
#include "dna/telemetry.hpp"

namespace dna {

struct ROBEntry {
    uint64_t seq;
    MicroOp op;
    bool ready = false;
    bool completed = false;
    uint64_t result_u64 = 0;
};

class ReorderBuffer {
public:
    ReorderBuffer(size_t capacity = 1024);
    uint64_t push(const MicroOp &m);
    std::optional<ROBEntry> peek_commit();
    bool mark_completed(uint64_t seq, uint64_t result = 0);
    void commit_head(std::function<void(const ROBEntry&)> commit_cb);
    size_t size();
private:
    std::vector<ROBEntry> buf_;
    size_t head_;
    size_t tail_;
    size_t capacity_;
    uint64_t next_seq_;
    std::mutex mtx_;
};

struct RenameMap {
    std::unordered_map<uint32_t, uint64_t> map;
    std::mutex mtx;
    void set(uint32_t arch, uint64_t seq);
    std::optional<uint64_t> get(uint32_t arch);
    void clear(uint32_t arch);
};

class ReservationStation {
public:
    ReservationStation(size_t cap = 4096);
    bool push(uint64_t rob_seq, const MicroOp &op);
    bool pop_ready(uint64_t &out_seq, MicroOp &out_op);
    size_t size();
private:
    struct RSItem { uint64_t seq; MicroOp op; bool issued=false; };
    std::vector<RSItem> items_;
    std::mutex mtx_;
};

class OOOEngine {
public:
    OOOEngine(size_t rob_cap = 4096, size_t rs_cap = 4096, size_t workers = 4);
    ~OOOEngine();

    uint64_t dispatch(const MicroOp &op);
    void start();
    void stop();
    void set_executor(std::function<void(const MicroOp&, uint64_t)> exec_cb);
    bool mark_completed(uint64_t seq, uint64_t result = 0);
    Telemetry &telemetry();
private:
    ReorderBuffer rob_;
    ReservationStation rs_;
    RenameMap rename_;
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{false};
    std::function<void(const MicroOp&, uint64_t)> exec_cb_;
    void worker_loop();
    Telemetry telemetry_;
};

} // namespace dna
EOM

# ============================
# 3) OOO MODULE IMPLEMENTATION
# ============================
cat > src/dna/src/ooo.cpp << 'EOM'
#include "dna/ooo.hpp"
#include <cassert>
#include <chrono>
#include <thread>
#include <cstring>

namespace dna {

// --- ROB ---
ReorderBuffer::ReorderBuffer(size_t capacity)
: buf_(capacity), head_(0), tail_(0), capacity_(capacity), next_seq_(1) {}

uint64_t ReorderBuffer::push(const MicroOp &m) {
    std::lock_guard lock(mtx_);
    size_t next_tail = (tail_ + 1) % capacity_;
    assert(next_tail != head_ && "ROB full");
    uint64_t seq = next_seq_++;
    buf_[tail_] = ROBEntry{seq, m, false, false, 0};
    tail_ = next_tail;
    return seq;
}

std::optional<ROBEntry> ReorderBuffer::peek_commit() {
    std::lock_guard lock(mtx_);
    if (head_ == tail_) return std::nullopt;
    auto &e = buf_[head_];
    if (e.completed) return e;
    return std::nullopt;
}

bool ReorderBuffer::mark_completed(uint64_t seq, uint64_t result) {
    std::lock_guard lock(mtx_);
    for (size_t i=0;i<capacity_;++i) {
        size_t idx = (head_ + i) % capacity_;
        if (buf_[idx].seq == seq) {
            buf_[idx].completed = true;
            buf_[idx].result_u64 = result;
            return true;
        }
    }
    return false;
}

void ReorderBuffer::commit_head(std::function<void(const ROBEntry&)> commit_cb) {
    std::lock_guard lock(mtx_);
    while (head_ != tail_ && buf_[head_].completed) {
        commit_cb(buf_[head_]);
        buf_[head_].completed = false;
        buf_[head_].ready = false;
        head_ = (head_ + 1) % capacity_;
    }
}

size_t ReorderBuffer::size() {
    std::lock_guard lock(mtx_);
    return (tail_ + capacity_ - head_) % capacity_;
}

// --- RenameMap ---
void RenameMap::set(uint32_t arch, uint64_t seq) {
    std::lock_guard lock(mtx);
    map[arch] = seq;
}
std::optional<uint64_t> RenameMap::get(uint32_t arch) {
    std::lock_guard lock(mtx);
    auto it = map.find(arch);
    if (it == map.end()) return std::nullopt;
    return it->second;
}
void RenameMap::clear(uint32_t arch) {
    std::lock_guard lock(mtx);
    map.erase(arch);
}

// --- ReservationStation ---
ReservationStation::ReservationStation(size_t cap) {
    items_.reserve(cap);
}
bool ReservationStation::push(uint64_t rob_seq, const MicroOp &op) {
    std::lock_guard lock(mtx_);
    items_.push_back({rob_seq, op, false});
    return true;
}
bool ReservationStation::pop_ready(uint64_t &out_seq, MicroOp &out_op) {
    std::lock_guard lock(mtx_);
    for (auto &it : items_) {
        if (!it.issued) {
            it.issued = true;
            out_seq = it.seq;
            out_op = it.op;
            return true;
        }
    }
    return false;
}
size_t ReservationStation::size() {
    std::lock_guard lock(mtx_);
    return items_.size();
}

// --- OOOEngine ---
OOOEngine::OOOEngine(size_t rob_cap, size_t rs_cap, size_t workers)
: rob_(rob_cap), rs_(rs_cap) {}

OOOEngine::~OOOEngine() { stop(); }

uint64_t OOOEngine::dispatch(const MicroOp &op) {
    uint64_t seq = rob_.push(op);

    // simplistic rename (if arg0 used as dest)
    if (op.type == MicroOpType::ADD || op.type == MicroOpType::POPCOUNT) {
        rename_.set((uint32_t)(op.arg0 & 0xffffffff), seq);
    }

    rs_.push(seq, op);
    telemetry_.increment_counter("ooo_dispatched");
    return seq;
}

void OOOEngine::start() {
    if (running_.exchange(true)) return;
    for (int i=0;i<4;++i)
        workers_.emplace_back([this]{ worker_loop(); });
}

void OOOEngine::stop() {
    if (!running_.exchange(false)) return;
    for (auto &t: workers_) if (t.joinable()) t.join();
    workers_.clear();
}

void OOOEngine::set_executor(std::function<void(const MicroOp&, uint64_t)> exec_cb) {
    exec_cb_ = exec_cb;
}

bool OOOEngine::mark_completed(uint64_t seq, uint64_t result) {
    return rob_.mark_completed(seq, result);
}

Telemetry &OOOEngine::telemetry() { return telemetry_; }

void OOOEngine::worker_loop() {
    while (running_) {
        uint64_t seq;
        MicroOp op;
        if (!rs_.pop_ready(seq, op)) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            continue;
        }

        if (exec_cb_) {
            exec_cb_(op, seq);
        } else {
            if (op.type == MicroOpType::POPCOUNT) {
                const uint64_t *data = (const uint64_t*)op.arg0;
                size_t n = (size_t)op.arg1;
                uint64_t total = 0;
                for (size_t i=0;i<n;++i) total += __builtin_popcountll(data[i]);
                rob_.mark_completed(seq, total);
            } else if (op.type == MicroOpType::MEMCOPY) {
                memcpy((void*)op.arg0, (const void*)op.arg1, (size_t)op.arg2);
                rob_.mark_completed(seq, 0);
            } else {
                rob_.mark_completed(seq, 0);
            }
        }
        telemetry_.increment_counter("ooo_executed");

        rob_.commit_head([this](const ROBEntry &e){
            if (e.op.type == MicroOpType::ADD || e.op.type == MicroOpType::POPCOUNT)
                rename_.clear((uint32_t)(e.op.arg0 & 0xffffffff));
            telemetry_.increment_counter("ooo_committed");
        });
    }
}

} // namespace dna
EOM

# ============================
# 4) TESTE OOO
# ============================
cat > src/dna/src/tests/ooo_smoke_test.cpp << 'EOM'
#include "dna/ooo.hpp"
#include "dna/microop.hpp"
#include "dna/telemetry.hpp"
#include <iostream>
#include <vector>
#include <cstring>
#include <thread>
#include <chrono>

using namespace dna;

int main() {
    OOOEngine engine(4096, 4096, 4);

    engine.set_executor([&](const MicroOp &op, uint64_t seq){
        if (op.type == MicroOpType::POPCOUNT) {
            const uint64_t *data = (const uint64_t*)op.arg0;
            size_t n = (size_t)op.arg1;
            uint64_t total = 0;
            for (size_t i=0;i<n;++i) total += __builtin_popcountll(data[i]);
            engine.mark_completed(seq, total);
        } else if (op.type == MicroOpType::MEMCOPY) {
            memcpy((void*)op.arg0, (const void*)op.arg1, (size_t)op.arg2);
            engine.mark_completed(seq, 0);
        } else {
            engine.mark_completed(seq, 0);
        }
    });

    engine.start();

    std::vector<uint64_t> src(1024, 0xF0F0F0F0F0F0F0F0ull);
    std::vector<uint64_t> dst(1024, 0);

    for (int i=0;i<2000;i++) {
        MicroOp mop;
        mop.type = MicroOpType::POPCOUNT;
        mop.arg0 = (uint64_t)src.data();
        mop.arg1 = 1024;
        engine.dispatch(mop);
    }

    for (int i=0;i<2000;i++) {
        MicroOp mop;
        mop.type = MicroOpType::MEMCOPY;
        mop.arg0 = (uint64_t)dst.data();
        mop.arg1 = (uint64_t)src.data();
        mop.arg2 = 1024 * sizeof(uint64_t);
        engine.dispatch(mop);
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
    engine.stop();

    std::cout << "OOO smoke done. dispatched=" << engine.telemetry().get_counter("ooo_dispatched")
              << " executed=" << engine.telemetry().get_counter("ooo_executed")
              << " committed=" << engine.telemetry().get_counter("ooo_committed")
              << std::endl;

    return 0;
}
EOM

# ============================
# 5) CMakeLists patch
# ============================
if ! grep -q "ooo_smoke_test" src/dna/CMakeLists.txt; then
cat >> src/dna/CMakeLists.txt << 'EOM'

# --- OOOEngine smoke test
add_executable(ooo_smoke_test
    src/ooo.cpp
    src/tests/ooo_smoke_test.cpp
)
target_include_directories(ooo_smoke_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if (TARGET dna)
    target_link_libraries(ooo_smoke_test PRIVATE dna)
endif()
add_test(NAME ooo_smoke_test COMMAND ooo_smoke_test)
EOM
fi

# ============================
# 6) BUILD + TEST
# ============================
rm -rf build
cmake -S . -B build
cmake --build build -j"$(nproc)"

cd build
ctest --output-on-failure

echo ">>> COMPLETO. Verifique acima se todos os testes passaram."
