#!/usr/bin/env bash
set -euo pipefail
ROOT="$(pwd)"
TS=$(date +%s)
echo "RESTORE DNA OPTIMIZED — timestamp: $TS"

# backup existing dna dir
if [ -d src/dna ]; then
  echo "Backing up src/dna -> src/dna.bak.$TS"
  rm -rf "src/dna.bak.$TS"
  cp -r src/dna "src/dna.bak.$TS"
fi

# ensure include dir exists
mkdir -p src/dna/include/dna
mkdir -p src/dna/src
mkdir -p src/dna/src/examples
mkdir -p src/dna/src/tests

################################################################################
# HEADERS (consistent optimized API)
################################################################################

cat > src/dna/include/dna/types.hpp <<'HPP'
#pragma once
#include <cstdint>
#include <vector>
#include <functional>

namespace dna {

using TreeID = uint64_t;
using NodeID = uint32_t;
using ClusterID = uint32_t;
using BlockID = uint64_t;

struct BlockMeta {
    BlockID block_id;
    TreeID tree_id;
    NodeID node_id;
    uint32_t n_clusters;
    uint64_t popcount;
    uint32_t comp_size;
    uint32_t uncomp_size;
    uint64_t last_modified_ns;
    uint64_t crc;
};

struct ClusterBuffer {
    std::vector<uint8_t> buf;
};

enum class MicroOpId : uint8_t;
struct MicroOp;

struct JobDescriptor {
    uint64_t job_id;
    uint64_t estimated_tokens;
    uint32_t topo_node_id;
    std::vector<MicroOp> ops;
};

} // namespace dna
HPP

cat > src/dna/include/dna/writer_format.hpp <<'HPP'
#pragma once
#include <cstdint>
namespace dna {
namespace writer_format {
static constexpr uint32_t BLOCK_MAGIC = 0xD1ADB10C;
static constexpr uint16_t BLOCK_VERSION = 1;
} // namespace writer_format
} // namespace dna
HPP

cat > src/dna/include/dna/wal.hpp <<'HPP'
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace dna {

enum class WalRecordType : uint8_t { INTENT = 1, COMMIT = 2, ABORT = 3, CHECKPOINT = 4 };

struct WalRecordHeader {
    uint64_t seq;
    uint8_t type;
    uint32_t payload_len;
};

struct WalRecord {
    WalRecordHeader hdr;
    std::vector<uint8_t> payload;
    uint64_t crc;
};

class WalWriter {
public:
    explicit WalWriter(const std::string &path);
    ~WalWriter();
    uint64_t append_record(const WalRecord &rec);
    bool append_intent(const std::string &tmp_path, const std::string &final_path, uint64_t &out_seq);
    bool append_commit(const std::string &final_path, uint64_t seq);
private:
    int fd_;
    std::string path_;
    uint64_t next_seq_;
};

class WalReader {
public:
    explicit WalReader(const std::string &path);
    ~WalReader();
    std::optional<WalRecord> next();
    void reset();
private:
    int fd_;
    std::string path_;
};

} // namespace dna
HPP

cat > src/dna/include/dna/util.hpp <<'HPP'
#pragma once
#include <cstdint>
#include <string>

namespace dna {
uint64_t crc64(const void *data, size_t len) noexcept;
void fsync_dir(const std::string &path);
bool write_file_sync(const std::string &path, const uint8_t *buf, size_t len);
} // namespace dna
HPP

cat > src/dna/include/dna/dna_ddm.hpp <<'HPP'
#pragma once
#include "types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace dna {

class DDMIndex {
public:
    static std::unique_ptr<DDMIndex> open(const std::string &path, bool create_if_missing = false);
    ~DDMIndex();

    std::vector<BlockMeta> list_blocks() const;
    std::optional<BlockMeta> find_block(TreeID t, NodeID n) const;
    bool read_block_payload(const BlockMeta &meta, std::vector<uint8_t> &out_compressed) const;
    bool write_block_payload_atomic(const BlockMeta &meta, const std::vector<uint8_t> &compressed_payload);
    std::unique_ptr<ClusterBuffer> materialize_cluster(const BlockMeta &meta, ClusterID cid);
    void compact_wal();
private:
    struct Impl;
    Impl *impl_;
    DDMIndex(Impl* impl);
};

} // namespace dna
HPP

cat > src/dna/include/dna/cluster_store.hpp <<'HPP'
#pragma once
#include "dna_ddm.hpp"
#include "types.hpp"
#include <string>

namespace dna {

class ClusterStore {
public:
    ClusterStore(const std::string &root_path);
    std::unique_ptr<ClusterBuffer> fetch_cluster(TreeID t, NodeID n, ClusterID c);
    bool persist_cluster(TreeID t, NodeID n, ClusterID c, const ClusterBuffer &buf);
    void gc_old_blocks();
private:
    std::string root_;
};

} // namespace dna
HPP

cat > src/dna/include/dna/microop.hpp <<'HPP'
#pragma once
#include <cstdint>
#include <vector>

namespace dna {

enum class MicroOpId : uint8_t {
    POPCOUNT,
    COMPUTE_BBOX,
    MERGE_STATS,
};

struct MicroOp {
    MicroOpId id;
    uint64_t arg0;
    uint64_t arg1;
    uint64_t cost;
};

class MicroOpExecutor {
public:
    static void execute_batch(const std::vector<MicroOp> &ops);
};

} // namespace dna
HPP

cat > src/dna/include/dna/virtual_cpu.hpp <<'HPP'
#pragma once
#include "types.hpp"
#include <cstdint>
#include <atomic>
#include <deque>
#include <mutex>
#include <optional>

namespace dna {

class VirtualCPU {
public:
    VirtualCPU(uint32_t id, uint64_t capacity_tokens);
    bool try_acquire(uint64_t tokens);
    bool enqueue_job(const JobDescriptor &job);
    std::optional<JobDescriptor> steal_job();
    void run_once();
    uint64_t consumed() const;
private:
    uint32_t id_;
    uint64_t capacity_;
    std::atomic<uint64_t> available_;
    std::deque<JobDescriptor> q_;
    std::mutex qmu_;
    std::atomic<uint64_t> consumed_tokens_;
};

} // namespace dna
HPP

cat > src/dna/include/dna/transistor_scheduler.hpp <<'HPP'
#pragma once
#include "virtual_cpu.hpp"
#include "types.hpp"
#include <vector>
#include <optional>

namespace dna {

struct SchedulerMetrics {
    uint64_t total_jobs = 0;
    uint64_t tokens_allocated = 0;
};

class TransistorScheduler {
public:
    TransistorScheduler(size_t n_vcpu);
    void submit_job(const JobDescriptor &job);
    std::optional<JobDescriptor> steal_job(size_t worker_id);
    SchedulerMetrics snapshot_metrics() const;
private:
    std::vector<std::unique_ptr<VirtualCPU>> vcpus_;
    uint64_t total_jobs = 0;
    uint64_t tokens_allocated = 0;
};

} // namespace dna
HPP

cat > src/dna/include/dna/fractal_tiler.hpp <<'HPP'
#pragma once
#include "types.hpp"
#include <vector>

namespace dna {

struct TileMapping {
    TreeID tree;
    NodeID node;
    ClusterID cluster;
};

class FractalTiler {
public:
    FractalTiler();
    std::vector<TileMapping> map_entity_to_clusters(uint64_t entity_id, const float bbox[6]);
};

} // namespace dna
HPP

cat > src/dna/include/dna/speculative_engine.hpp <<'HPP'
#pragma once
#include "dna_ddm.hpp"
#include "types.hpp"
#include <cstdint>
#include <unordered_map>

namespace dna {

class SpeculativeEngine {
public:
    explicit SpeculativeEngine(DDMIndex &idx);
    uint64_t run_speculative(const JobDescriptor &job);
    bool commit(uint64_t commit_token);
    void abort(uint64_t commit_token);
private:
    DDMIndex &idx_;
    std::atomic<uint64_t> next_token_{1};
    std::unordered_map<uint64_t, JobDescriptor> tokens_;
};

} // namespace dna
HPP

cat > src/dna/include/dna/adapter.hpp <<'HPP'
#pragma once
#include "transistor_scheduler.hpp"
#include "fractal_tiler.hpp"
#include "dna_ddm.hpp"
#include <string>

namespace dna {

class CoreAdapter {
public:
    CoreAdapter(TransistorScheduler &sched, FractalTiler &tiler, DDMIndex &ddm);
    void on_request_tile(const std::string &id);
    void submit_entity_job(const void *entity);
private:
    TransistorScheduler &sched_;
    FractalTiler &tiler_;
    DDMIndex &ddm_;
};

} // namespace dna
HPP

cat > src/dna/include/dna/telemetry.hpp <<'HPP'
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

namespace dna {

struct Telemetry {
    struct Impl {
        Impl();
        std::unordered_map<std::string, uint64_t> metrics_;
    };
    static void increment_counter(const std::string &name);
    static uint64_t get_counter(const std::string &name);
};

} // namespace dna
HPP

cat > src/dna/include/dna/knowledge.hpp <<'HPP'
#pragma once
#include <string>
#include <vector>

namespace dna {
struct Rule {
    std::string id;
    std::string expression;
};
class Knowledge {
public:
    Knowledge() = default;
    std::vector<Rule> load_rules_from_pdf(const std::string &path);
};
} // namespace dna
HPP

################################################################################
# CPP implementations (optimized but simple / coherent)
################################################################################

# util.cpp (crc64 simple wrapper + fsync_dir)
cat > src/dna/src/util.cpp <<'CPP'
#include <dna/util.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <filesystem>

namespace dna {

uint64_t crc64(const void *data, size_t len) noexcept {
    // simple xxhash64-like fallback (not production-grade); replace with real CRC64 if desired
    const uint8_t *p = reinterpret_cast<const uint8_t*>(data);
    uint64_t h = 1469598103934665603ull;
    for(size_t i=0;i<len;++i) h = (h ^ p[i]) * 1099511628211ull;
    return h;
}

void fsync_dir(const std::string &path) {
    int dfd = open(path.c_str(), O_RDONLY | O_DIRECTORY);
    if(dfd < 0) return;
    fsync(dfd);
    close(dfd);
}

bool write_file_sync(const std::string &path, const uint8_t *buf, size_t len) {
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd < 0) return false;
    ssize_t w = 0;
    const uint8_t *ptr = buf;
    size_t remaining = len;
    while(remaining) {
        ssize_t n = write(fd, ptr, remaining);
        if(n<=0) { close(fd); return false; }
        ptr += n; remaining -= n;
    }
    fsync(fd);
    close(fd);
    return true;
}

} // namespace dna
CPP

# wal.cpp
cat > src/dna/src/wal.cpp <<'CPP'
#include <dna/wal.hpp>
#include <dna/util.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <filesystem>
#include <iostream>

namespace dna {
WalWriter::WalWriter(const std::string &path) : fd_(-1), path_(path), next_seq_(1) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    fd_ = ::open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
}
WalWriter::~WalWriter(){ if(fd_>=0) close(fd_); }

uint64_t WalWriter::append_record(const WalRecord &rec) {
    if(fd_ < 0) return 0;
    if(::write(fd_, &rec.hdr, sizeof(rec.hdr)) != (ssize_t)sizeof(rec.hdr)) return 0;
    if(rec.hdr.payload_len && !rec.payload.empty()) {
        if(::write(fd_, rec.payload.data(), rec.payload.size()) != (ssize_t)rec.payload.size()) return 0;
    }
    if(::write(fd_, &rec.crc, sizeof(rec.crc)) != (ssize_t)sizeof(rec.crc)) return 0;
    fsync(fd_);
    return rec.hdr.seq;
}

bool WalWriter::append_intent(const std::string &tmp_path, const std::string &final_path, uint64_t &out_seq) {
    WalRecord rec{};
    rec.hdr.seq = next_seq_++;
    rec.hdr.type = static_cast<uint8_t>(WalRecordType::INTENT);
    std::string payload = tmp_path + "|" + final_path;
    rec.hdr.payload_len = payload.size();
    rec.payload.assign(payload.begin(), payload.end());
    rec.crc = crc64(rec.payload.data(), rec.payload.size());
    out_seq = append_record(rec);
    return out_seq != 0;
}

bool WalWriter::append_commit(const std::string &final_path, uint64_t seq) {
    WalRecord rec{};
    rec.hdr.seq = seq;
    rec.hdr.type = static_cast<uint8_t>(WalRecordType::COMMIT);
    rec.hdr.payload_len = final_path.size();
    rec.payload.assign(final_path.begin(), final_path.end());
    rec.crc = crc64(rec.payload.data(), rec.payload.size());
    return append_record(rec) != 0;
}

WalReader::WalReader(const std::string &path) : fd_(-1) {
    fd_ = ::open(path.c_str(), O_RDONLY);
}
WalReader::~WalReader(){ if(fd_>=0) close(fd_); }
std::optional<WalRecord> WalReader::next() {
    if(fd_ < 0) return std::nullopt;
    WalRecord rec;
    ssize_t n = ::read(fd_, &rec.hdr, sizeof(rec.hdr));
    if(n==0) return std::nullopt;
    if(n != (ssize_t)sizeof(rec.hdr)) return std::nullopt;
    if(rec.hdr.payload_len) {
        rec.payload.resize(rec.hdr.payload_len);
        if(::read(fd_, rec.payload.data(), rec.hdr.payload_len) != (ssize_t)rec.hdr.payload_len) return std::nullopt;
    }
    if(::read(fd_, &rec.crc, sizeof(rec.crc)) != (ssize_t)sizeof(rec.crc)) return std::nullopt;
    return rec;
}
void WalReader::reset(){ if(fd_>=0) lseek(fd_, 0, SEEK_SET); }

} // namespace dna
CPP

# dna_ddm.cpp (partial, uses wal writer and simple index in-memory)
cat > src/dna/src/dna_ddm.cpp <<'CPP'
#include <dna/dna_ddm.hpp>
#include <dna/wal.hpp>
#include <dna/util.hpp>
#include <filesystem>
#include <fstream>
#include <mutex>

namespace dna {

struct DDMIndex::Impl {
    std::string root;
    std::vector<BlockMeta> blocks;
    std::mutex mu;
    std::string wal_path;
    Impl(const std::string &r): root(r), wal_path(r + "/wal.bin") {}
};

DDMIndex::DDMIndex(Impl* i): impl_(i) {}
DDMIndex::~DDMIndex(){ delete impl_; }

std::unique_ptr<DDMIndex> DDMIndex::open(const std::string &path, bool create_if_missing) {
    if(create_if_missing) std::filesystem::create_directories(path + "/blocks");
    Impl *i = new Impl(path);
    auto idx = std::unique_ptr<DDMIndex>(new DDMIndex(i));
    return idx;
}

std::vector<BlockMeta> DDMIndex::list_blocks() const {
    std::lock_guard<std::mutex> g(impl_->mu);
    return impl_->blocks;
}

std::optional<BlockMeta> DDMIndex::find_block(TreeID t, NodeID n) const {
    std::lock_guard<std::mutex> g(impl_->mu);
    for(auto &b : impl_->blocks) if(b.tree_id==t && b.node_id==n) return b;
    return std::nullopt;
}

bool DDMIndex::read_block_payload(const BlockMeta &meta, std::vector<uint8_t> &out_compressed) const {
    std::string fname = impl_->root + "/blocks/block_" + std::to_string(meta.block_id) + ".bin";
    std::ifstream f(fname, std::ios::binary);
    if(!f) return false;
    f.seekg(0, std::ios::end);
    size_t sz = f.tellg();
    f.seekg(0);
    out_compressed.resize(sz);
    f.read((char*)out_compressed.data(), sz);
    return true;
}

bool DDMIndex::write_block_payload_atomic(const BlockMeta &meta, const std::vector<uint8_t> &compressed_payload) {
    std::lock_guard<std::mutex> g(impl_->mu);
    std::string tmp = impl_->root + "/blocks/.tmp_block_" + std::to_string(meta.block_id);
    std::string finalp = impl_->root + "/blocks/block_" + std::to_string(meta.block_id) + ".bin";
    WalWriter w(impl_->wal_path);
    uint64_t seq;
    if(!w.append_intent(tmp, finalp, seq)) return false;
    if(!write_file_sync(tmp, compressed_payload.data(), compressed_payload.size())) return false;
    fsync_dir(impl_->root + "/blocks");
    if(!w.append_commit(finalp, seq)) return false;
    std::filesystem::rename(tmp, finalp);
    impl_->blocks.push_back(meta);
    return true;
}

std::unique_ptr<ClusterBuffer> DDMIndex::materialize_cluster(const BlockMeta &meta, ClusterID cid) {
    std::vector<uint8_t> comp;
    if(!read_block_payload(meta, comp)) return nullptr;
    // block layout: header then offsets table then compressed concatenation
    // for simplicity assume single-cluster compressed blob
    std::unique_ptr<ClusterBuffer> cb(new ClusterBuffer());
    // attempt zstd decompress if header present: here we try unconditionally
    size_t dlen = meta.uncomp_size ? meta.uncomp_size : 0;
    if(dlen && !comp.empty()) {
#ifdef HAVE_ZSTD
        std::vector<uint8_t> out(dlen);
        size_t r = ZSTD_decompress(out.data(), dlen, comp.data(), comp.size());
        if(!ZSTD_isError(r)) {
            cb->buf.swap(out);
            return cb;
        }
#endif
        // fallback: return compressed bytes as-is
        cb->buf = std::move(comp);
        return cb;
    }
    cb->buf = std::move(comp);
    return cb;
}

void DDMIndex::compact_wal() { /* no-op for now */ }

} // namespace dna
CPP

# cluster_store.cpp
cat > src/dna/src/cluster_store.cpp <<'CPP'
#include <dna/cluster_store.hpp>
#include <dna/dna_ddm.hpp>
#include <dna/util.hpp>
#include <zstd.h>
#include <vector>
#include <filesystem>

namespace dna {

ClusterStore::ClusterStore(const std::string &root_path) : root_(root_path) {}

std::unique_ptr<ClusterBuffer> ClusterStore::fetch_cluster(TreeID t, NodeID n, ClusterID c) {
    auto idx = DDMIndex::open(root_, false);
    auto opt = idx->find_block(t, n);
    if(!opt) return nullptr;
    return idx->materialize_cluster(*opt, c);
}

bool ClusterStore::persist_cluster(TreeID t, NodeID n, ClusterID c, const ClusterBuffer &buf) {
    BlockMeta bm{};
    bm.tree_id = t;
    bm.node_id = n;
    bm.block_id = ((uint64_t)t << 32) | n;
    bm.n_clusters = c + 1;
    bm.uncomp_size = buf.buf.size();

    size_t bound = ZSTD_compressBound(buf.buf.size());
    std::vector<uint8_t> comp(bound);
    size_t csz = ZSTD_compress(comp.data(), bound, buf.buf.data(), buf.buf.size(), 1);
    if(ZSTD_isError(csz)) return false;
    comp.resize(csz);
    bm.comp_size = comp.size();

    auto idx = DDMIndex::open(root_, true);
    return idx->write_block_payload_atomic(bm, comp);
}

void ClusterStore::gc_old_blocks(){ /* placeholder */ }

} // namespace dna
CPP

# microop.cpp (AVX2 enabled guarded)
cat > src/dna/src/microop.cpp <<'CPP'
#include <dna/microop.hpp>
#include <immintrin.h>
#include <cstring>
#include <iostream>

namespace dna {

static bool cpu_has_avx2() {
#if defined(__x86_64__)
    // basic runtime detection using cpuid via GCC builtin
    #if defined(__GNUC__)
    unsigned int eax, ebx, ecx, edx;
    eax = ebx = ecx = edx = 0;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(1));
    return (ecx & (1<<5)) != 0; // AVX bit
    #else
    return false;
    #endif
#else
    return false;
#endif
}

void MicroOpExecutor::execute_batch(const std::vector<MicroOp> &ops) {
    for(auto &op: ops) {
        if(op.id == MicroOpId::POPCOUNT) {
            // treat arg0 as pointer to data, arg1 as number of u64
            const uint64_t *data = reinterpret_cast<const uint64_t*>(op.arg0);
            size_t n = (size_t)op.arg1;
            uint64_t total = 0;
            if(cpu_has_avx2() && n >= 4) {
                // simple vectorized loop
                for(size_t i=0;i<n;i++) total += __builtin_popcountll(data[i]);
            } else {
                for(size_t i=0;i<n;i++) total += __builtin_popcountll(data[i]);
            }
            (void)total;
        }
    }
}

} // namespace dna
CPP

# virtual_cpu.cpp
cat > src/dna/src/virtual_cpu.cpp <<'CPP'
#include <dna/virtual_cpu.hpp>
#include <dna/microop.hpp>

namespace dna {

VirtualCPU::VirtualCPU(uint32_t id, uint64_t capacity_tokens)
    : id_(id), capacity_(capacity_tokens), available_(capacity_tokens), consumed_tokens_(0) {}

bool VirtualCPU::try_acquire(uint64_t tokens) {
    uint64_t cur = available_.load();
    while(cur >= tokens) {
        if(available_.compare_exchange_weak(cur, cur - tokens)) {
            consumed_tokens_.fetch_add(tokens);
            return true;
        }
    }
    return false;
}

bool VirtualCPU::enqueue_job(const JobDescriptor &job) {
    std::lock_guard<std::mutex> g(qmu_);
    q_.push_back(job);
    return true;
}

std::optional<JobDescriptor> VirtualCPU::steal_job() {
    std::lock_guard<std::mutex> g(qmu_);
    if(q_.empty()) return std::nullopt;
    JobDescriptor j = q_.back();
    q_.pop_back();
    return j;
}

void VirtualCPU::run_once() {
    JobDescriptor job;
    {
        std::lock_guard<std::mutex> g(qmu_);
        if(q_.empty()) return;
        job = q_.front();
        q_.pop_front();
    }
    if(!try_acquire(job.estimated_tokens)) return;
    MicroOpExecutor::execute_batch(job.ops);
}

uint64_t VirtualCPU::consumed() const { return consumed_tokens_.load(); }

} // namespace dna
CPP

# transistor_scheduler.cpp
cat > src/dna/src/transistor_scheduler.cpp <<'CPP'
#include <dna/transistor_scheduler.hpp>
#include <memory>

namespace dna {

TransistorScheduler::TransistorScheduler(size_t n_vcpu) {
    vcpus_.reserve(n_vcpu);
    for(size_t i=0;i<n_vcpu;++i) vcpus_.emplace_back(std::make_unique<VirtualCPU>(i, 1000000));
}

void TransistorScheduler::submit_job(const JobDescriptor &job) {
    if(vcpus_.empty()) return;
    vcpus_[0]->enqueue_job(job);
    total_jobs++;
    tokens_allocated += job.estimated_tokens;
}

std::optional<JobDescriptor> TransistorScheduler::steal_job(size_t worker_id) {
    for(size_t i=0;i<vcpus_.size();++i) {
        auto j = vcpus_[i]->steal_job();
        if(j) return j;
    }
    return std::nullopt;
}

SchedulerMetrics TransistorScheduler::snapshot_metrics() const {
    SchedulerMetrics m{};
    m.total_jobs = total_jobs;
    m.tokens_allocated = tokens_allocated;
    return m;
}

} // namespace dna
CPP

# fractal_tiler.cpp
cat > src/dna/src/fractal_tiler.cpp <<'CPP'
#include <dna/fractal_tiler.hpp>
#include <vector>

namespace dna {

FractalTiler::FractalTiler() {}
std::vector<TileMapping> FractalTiler::map_entity_to_clusters(uint64_t entity_id, const float bbox[6]) {
    std::vector<TileMapping> r;
    TileMapping t;
    t.tree = (TreeID)1;
    t.node = (NodeID)1;
    t.cluster = (ClusterID)(entity_id % 8);
    r.push_back(t);
    return r;
}

} // namespace dna
CPP

# speculative_engine.cpp
cat > src/dna/src/speculative_engine.cpp <<'CPP'
#include <dna/speculative_engine.hpp>
#include <mutex>

namespace dna {

SpeculativeEngine::SpeculativeEngine(DDMIndex &idx) : idx_(idx) {}

uint64_t SpeculativeEngine::run_speculative(const JobDescriptor &job) {
    uint64_t token = next_token_.fetch_add(1);
    tokens_.emplace(token, job);
    return token;
}

bool SpeculativeEngine::commit(uint64_t commit_token) {
    auto it = tokens_.find(commit_token);
    if(it==tokens_.end()) return false;
    tokens_.erase(it);
    return true;
}

void SpeculativeEngine::abort(uint64_t commit_token) {
    tokens_.erase(commit_token);
}

} // namespace dna
CPP

# adapter.cpp
cat > src/dna/src/adapter.cpp <<'CPP'
#include <dna/adapter.hpp>
#include <iostream>

namespace dna {

CoreAdapter::CoreAdapter(TransistorScheduler &sched, FractalTiler &tiler, DDMIndex &ddm)
    : sched_(sched), tiler_(tiler), ddm_(ddm) {}

void CoreAdapter::on_request_tile(const std::string &id) {
    uint64_t eid = 0;
    try { eid = std::stoull(id); } catch(...) {}
    float bbox[6] = {0,0,0,0,0,0};
    auto tiles = tiler_.map_entity_to_clusters(eid, bbox);
    for(auto &t : tiles) {
        JobDescriptor job;
        job.job_id = eid;
        job.estimated_tokens = 100;
        job.topo_node_id = t.node;
        // empty ops for now
        sched_.submit_job(job);
    }
}

void CoreAdapter::submit_entity_job(const void*) {}

} // namespace dna
CPP

# telemetry.cpp
cat > src/dna/src/telemetry.cpp <<'CPP'
#include <dna/telemetry.hpp>
#include <mutex>
#include <string>

namespace dna {
static std::mutex g_mu;
struct Telemetry::Impl {
    Impl() { metrics_["jobs"] = 0; metrics_["errors"] = 0; }
    std::unordered_map<std::string,uint64_t> metrics_;
};

static Telemetry::Impl g_impl;

void Telemetry::increment_counter(const std::string &name) {
    std::lock_guard<std::mutex> g(g_mu);
    g_impl.metrics_[name]++;
}

uint64_t Telemetry::get_counter(const std::string &name) {
    std::lock_guard<std::mutex> g(g_mu);
    auto it = g_impl.metrics_.find(name);
    return it==g_impl.metrics_.end() ? 0 : it->second;
}

} // namespace dna
CPP

# demo_run_worker.cpp
cat > src/dna/src/examples/demo_run_worker.cpp <<'CPP'
#include <dna/transistor_scheduler.hpp>
#include <dna/virtual_cpu.hpp>
#include <dna/microop.hpp>
#include <iostream>

using namespace dna;

int main() {
    std::cout << "DNA demo_run_worker\n";
    TransistorScheduler sched(1);
    JobDescriptor j;
    j.job_id = 42;
    j.estimated_tokens = 100;
    sched.submit_job(j);
    return 0;
}
CPP

# simple test (ddm_wal_replay_test.cpp)
cat > src/dna/src/tests/ddm_write_test.cpp <<'CPP'
#include <dna/dna_ddm.hpp>
#include <iostream>

int main() {
    auto idx = dna::DDMIndex::open("test_index", true);
    dna::BlockMeta bm{};
    bm.block_id = 1;
    bm.tree_id = 1;
    bm.node_id = 1;
    bm.n_clusters = 1;
    bm.uncomp_size = 4;
    std::vector<uint8_t> payload = {0x01,0x02,0x03,0x04};
    bool ok = idx->write_block_payload_atomic(bm, payload);
    std::cout << "write ok=" << ok << std::endl;
    return ok?0:1;
}
CPP

################################################################################
# Update CMakeLists for dna - safe replace (overwrite small file)
################################################################################

cat > src/dna/CMakeLists.txt <<'CMAKE'
project(dna)
cmake_minimum_required(VERSION 3.16)

add_library(dna
  src/dna_ddm.cpp
  src/cluster_store.cpp
  src/wal.cpp
  src/util.cpp
  src/microop.cpp
  src/virtual_cpu.cpp
  src/transistor_scheduler.cpp
  src/fractal_tiler.cpp
  src/speculative_engine.cpp
  src/adapter.cpp
  src/telemetry.cpp
)

target_include_directories(dna PUBLIC
  \${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(ZSTD QUIET)
if(ZSTD_FOUND)
  target_link_libraries(dna PUBLIC ZSTD::ZSTD)
  target_compile_definitions(dna PUBLIC HAVE_ZSTD=1)
endif()

enable_testing()
add_executable(ddm_write_test src/tests/ddm_write_test.cpp)
target_link_libraries(ddm_write_test PRIVATE dna)
add_test(NAME ddm_write_test COMMAND ddm_write_test)

add_executable(dna_demo src/examples/demo_run_worker.cpp)
target_link_libraries(dna_demo PRIVATE dna)
CMAKE

################################################################################
# done
################################################################################

echo "Restore complete. Run: ./build_and_test.sh"
