#pragma once

#include "Kernel.h"
#include "ComputeDevice.h"
#include <chrono>
#include <cstdint>
#include <functional>

namespace StonePotatoes {
namespace GPU {

// CPU reference backend — REAL implementation.
// Dispatches kernels across a std::thread pool, partitioning the
// global thread range into contiguous chunks per worker.
// CPU 參考後端 — 真實實作。
// 以 std::thread 池 dispatch 核心，將全域執行緒範圍
// 切分為連續區段分配給各 worker。
class CPUBackend {
public:
    struct Stats {
        double dispatchMs = 0.0;
        uint32_t threadsUsed = 0;
        uint32_t itemsProcessed = 0;
    };

    explicit CPUBackend(uint32_t workerCount = 0);
    ~CPUBackend();

    // Blocking dispatch: partitions [0, threadCount) across workers.
    // 阻塞式 dispatch：將 [0, threadCount) 切分給各 worker。
    Stats Dispatch(const Kernel& kernel, const DispatchParams& params);

    // Convenience: parallel_for over an index range.
    // 便捷函式：索引範圍的 parallel_for。
    Stats ParallelFor(uint32_t count,
                      const std::function<void(uint32_t)>& fn,
                      uint32_t grainSize = 1024);

    uint32_t WorkerCount() const { return m_Workers; }
    const DeviceCaps& GetCaps() const { return m_Caps; }

private:
    uint32_t m_Workers;
    DeviceCaps m_Caps;
};

} // namespace GPU
} // namespace StonePotatoes
