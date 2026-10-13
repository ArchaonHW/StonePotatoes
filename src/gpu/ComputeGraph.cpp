#include "ComputeGraph.h"
#include <unordered_set>
#include <chrono>
#include <future>

namespace StonePotatoes {
namespace GPU {

std::shared_ptr<ComputeNode>
ComputeNode::MakeKernel(std::string name, Kernel k, uint32_t threads,
                        void* userData) {
    auto n = std::shared_ptr<ComputeNode>(
        new ComputeNode(std::move(name), Kind::KernelDispatch));
    n->kernel = std::move(k);
    n->threads = threads;
    n->userData = userData;
    return n;
}

std::shared_ptr<ComputeNode>
ComputeNode::MakeTask(std::string name, std::function<void()> fn) {
    auto n = std::shared_ptr<ComputeNode>(
        new ComputeNode(std::move(name), Kind::HostTask));
    n->task = std::move(fn);
    return n;
}

std::shared_ptr<ComputeNode> ComputeNode::MakeBarrier(std::string name) {
    return std::shared_ptr<ComputeNode>(
        new ComputeNode(std::move(name), Kind::Barrier));
}

void ComputeNode::AddDependency(const std::shared_ptr<ComputeNode>& dep) {
    m_Deps.push_back(dep);
}

void ComputeGraph::AddNode(const std::shared_ptr<ComputeNode>& node) {
    m_Nodes.push_back(node);
}

void ComputeGraph::Clear() { m_Nodes.clear(); }

ComputeGraph::RunResult ComputeGraph::Execute(CPUBackend& backend) {
    RunResult r;
    auto t0 = std::chrono::steady_clock::now();

    // Kahn's algorithm over the dependency DAG.
    // 對相依 DAG 執行 Kahn 演算法。
    std::unordered_set<ComputeNode*> done;
    std::unordered_set<ComputeNode*> visiting;

    // Iterative topological execution: repeatedly pick ready nodes.
    // 迭代式拓撲執行：重複挑選就緒節點。
    size_t executed = 0;
    const size_t total = m_Nodes.size();
    while (executed < total) {
        bool progressed = false;
        for (auto& node : m_Nodes) {
            if (done.count(node.get())) continue;

            bool ready = true;
            for (auto& dep : node->Dependencies()) {
                if (!done.count(dep.get())) {
                    if (visiting.count(node.get()) &&
                        dep.get() == node.get()) {
                        r.cycleDetected = true;
                        r.cycleNode = node->GetName();
                        return r;
                    }
                    ready = false;
                    break;
                }
            }
            if (!ready) continue;

            visiting.insert(node.get());
            switch (node->GetKind()) {
            case ComputeNode::Kind::KernelDispatch: {
                DispatchParams p;
                p.threadCount = node->threads;
                p.userData = node->userData;
                backend.Dispatch(node->kernel, p);
                break;
            }
            case ComputeNode::Kind::HostTask:
                if (node->task) node->task();
                break;
            case ComputeNode::Kind::Barrier:
                break;
            }
            visiting.erase(node.get());
            done.insert(node.get());
            r.executionOrder.push_back(node->GetName());
            ++executed;
            progressed = true;
        }
        if (!progressed) {
            // Remaining nodes have unmet deps — a cycle must exist.
            // 剩餘節點皆有未滿足的相依 — 必存在循環。
            r.cycleDetected = true;
            for (auto& node : m_Nodes) {
                if (!done.count(node.get())) {
                    r.cycleNode = node->GetName();
                    break;
                }
            }
            return r;
        }
    }

    r.ok = true;
    r.nodesExecuted = static_cast<uint32_t>(executed);
    r.totalMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();
    return r;
}

ComputeGraph::RunResult
ComputeGraph::ExecuteParallel(CPUBackend& backend) {
    RunResult r;
    auto t0 = std::chrono::steady_clock::now();

    std::unordered_set<ComputeNode*> done;
    const size_t total = m_Nodes.size();
    size_t executed = 0;

    while (executed < total) {
        // Collect all nodes whose dependencies are satisfied.
        // 收集所有相依已滿足的節點。
        std::vector<std::shared_ptr<ComputeNode>> ready;
        for (auto& node : m_Nodes) {
            if (done.count(node.get())) continue;
            bool ok = true;
            for (auto& dep : node->Dependencies()) {
                if (!done.count(dep.get())) { ok = false; break; }
            }
            if (ok) ready.push_back(node);
        }
        if (ready.empty()) {
            r.cycleDetected = true;
            for (auto& node : m_Nodes) {
                if (!done.count(node.get())) {
                    r.cycleNode = node->GetName();
                    break;
                }
            }
            return r;
        }

        // Dispatch kernel nodes concurrently; tasks run inline after.
        // 並行派發 kernel 節點；task 節點隨後內聯執行。
        std::vector<std::future<CPUBackend::Stats>> futs;
        futs.reserve(ready.size());
        for (auto& node : ready) {
            if (node->GetKind() == ComputeNode::Kind::KernelDispatch) {
                DispatchParams p;
                p.threadCount = node->threads;
                p.userData = node->userData;
                futs.push_back(backend.DispatchAsync(node->kernel, p));
            }
        }
        for (auto& f : futs) f.get();

        for (auto& node : ready) {
            if (node->GetKind() == ComputeNode::Kind::HostTask && node->task)
                node->task();
            done.insert(node.get());
            r.executionOrder.push_back(node->GetName());
            ++executed;
        }
    }

    r.ok = true;
    r.nodesExecuted = static_cast<uint32_t>(executed);
    r.totalMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();
    return r;
}

} // namespace GPU
} // namespace StonePotatoes
