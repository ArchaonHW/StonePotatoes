#include "CPUBackend.h"
#include <algorithm>
#include <thread>
#include <vector>
#include <atomic>

namespace StonePotatoes {
namespace GPU {

CPUBackend::CPUBackend(uint32_t workerCount) {
    uint32_t hw = std::thread::hardware_concurrency();
    m_Workers = workerCount ? workerCount : (hw ? hw : 1);
    m_Caps = EnumerateDevices().front();
}

CPUBackend::~CPUBackend() = default;

CPUBackend::Stats CPUBackend::Dispatch(const Kernel& kernel,
                                       const DispatchParams& params) {
    Stats s;
    if (!kernel.IsValid() || params.threadCount == 0) return s;

    auto t0 = std::chrono::steady_clock::now();
    const uint32_t total = params.threadCount;
    const uint32_t workers = std::min(m_Workers, total);

    std::vector<std::thread> pool;
    pool.reserve(workers);
    const uint32_t chunk = total / workers;
    const uint32_t rem = total % workers;

    uint32_t begin = 0;
    for (uint32_t w = 0; w < workers; ++w) {
        const uint32_t size = chunk + (w < rem ? 1 : 0);
        const uint32_t end = begin + size;
        pool.emplace_back([begin, end, &kernel, ud = params.userData] {
            for (uint32_t i = begin; i < end; ++i) {
                kernel.Invoke(i, ud);
            }
        });
        begin = end;
    }
    for (auto& t : pool) t.join();

    s.dispatchMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();
    s.threadsUsed = workers;
    s.itemsProcessed = total;
    return s;
}

std::future<CPUBackend::Stats>
CPUBackend::DispatchAsync(const Kernel& kernel,
                          const DispatchParams& params) {
    return std::async(std::launch::async,
                      [this, &kernel, params] {
                          return Dispatch(kernel, params);
                      });
}

CPUBackend::Stats CPUBackend::ParallelFor(
    uint32_t count, const std::function<void(uint32_t)>& fn,
    uint32_t grainSize) {
    // Grain partitioning: each worker grabs grain-sized slices via
    // atomic counter for better load balance with uneven work.
    // 粒度切分：worker 透過原子計數器抓取粒度大小的片段，
    // 對不均勻工作負載有更好的負載平衡。
    Stats s;
    if (count == 0 || !fn) return s;
    auto t0 = std::chrono::steady_clock::now();

    std::atomic<uint32_t> next{0};
    const uint32_t workers = std::min(m_Workers, count);
    const uint32_t grain = grainSize ? grainSize : 1;

    std::vector<std::thread> pool;
    pool.reserve(workers);
    for (uint32_t w = 0; w < workers; ++w) {
        pool.emplace_back([&next, count, grain, &fn] {
            while (true) {
                uint32_t begin = next.fetch_add(grain);
                if (begin >= count) break;
                uint32_t end = std::min(begin + grain, count);
                for (uint32_t i = begin; i < end; ++i) fn(i);
            }
        });
    }
    for (auto& t : pool) t.join();

    s.dispatchMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();
    s.threadsUsed = workers;
    s.itemsProcessed = count;
    return s;
}

} // namespace GPU
} // namespace StonePotatoes
