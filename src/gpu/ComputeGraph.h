#pragma once

#include "Kernel.h"
#include "CPUBackend.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace GPU {

// Compute-graph node. Mirrors the IQGPU concept where AI, physics,
// rendering and simulation become schedulable compute nodes.
// 計算圖節點。對齊 IQGPU 概念 — AI、物理、渲染與模擬
// 皆為可排程的計算節點。
class ComputeNode {
public:
    enum class Kind {
        KernelDispatch,  // dispatch a Kernel over N threads
        HostTask,        // arbitrary CPU callback
        Barrier          // ordering fence
    };

    static std::shared_ptr<ComputeNode>
    MakeKernel(std::string name, Kernel kernel, uint32_t threads,
               void* userData = nullptr);

    static std::shared_ptr<ComputeNode>
    MakeTask(std::string name, std::function<void()> fn);

    static std::shared_ptr<ComputeNode> MakeBarrier(std::string name);

    const std::string& GetName() const { return m_Name; }
    Kind GetKind() const { return m_Kind; }

    void AddDependency(const std::shared_ptr<ComputeNode>& dep);
    const std::vector<std::shared_ptr<ComputeNode>>& Dependencies() const {
        return m_Deps;
    }

    Kernel kernel;
    uint32_t threads = 0;
    void* userData = nullptr;
    std::function<void()> task;

private:
    ComputeNode(std::string name, Kind k) : m_Name(std::move(name)), m_Kind(k) {}
    std::string m_Name;
    Kind m_Kind;
    std::vector<std::shared_ptr<ComputeNode>> m_Deps;
};

// Topological executor. Runs nodes on the CPU backend in dependency
// order; detects cycles.
// 拓撲執行器。依相依順序在 CPU 後端執行節點；偵測循環。
class ComputeGraph {
public:
    void AddNode(const std::shared_ptr<ComputeNode>& node);
    void Clear();

    struct RunResult {
        bool ok = false;
        bool cycleDetected = false;
        std::string cycleNode;
        double totalMs = 0.0;
        uint32_t nodesExecuted = 0;
        std::vector<std::string> executionOrder;
    };

    RunResult Execute(CPUBackend& backend);

    // Parallel variant: independent ready nodes are dispatched
    // concurrently (real multi-queue behavior on the CPU backend).
    // 平行版本：無相依的就緒節點並行派發
    // （CPU 後端上的真實多佇列行為）。
    RunResult ExecuteParallel(CPUBackend& backend);

    size_t NodeCount() const { return m_Nodes.size(); }

private:
    std::vector<std::shared_ptr<ComputeNode>> m_Nodes;
};

} // namespace GPU
} // namespace StonePotatoes
