#pragma once

#include "../gpu/ComputeDevice.h"
#include "../gpu/ComputeBuffer.h"
#include "../gpu/CPUBackend.h"
#include "../gpu/VulkanBackend.h"
#include "../gpu/ComputeGraph.h"
#include <memory>
#include <string>
#include <vector>

// IQGPU — Intelligent Quantum GPU Development System.
// Unified compute abstraction: the same submission API routes to
// whatever backend exists. Today that is the CPU reference backend;
// GPU/QPU slots are declared for future backends (Planned/Research).
// IQGPU — 智能量子 GPU 開發系統。
// 統一計算抽象：同一提交 API 路由至現有後端。
// 目前為 CPU 參考後端；GPU/QPU 槽位為未來後端保留
// （Planned/Research）。
namespace StonePotatoes {
namespace IQGPU {

class IQGPURuntime {
public:
    IQGPURuntime();
    ~IQGPURuntime();

    void Initialize();
    void Shutdown();
    bool IsInitialized() const { return m_Initialized; }

    // Device | 裝置
    const GPU::DeviceCaps& GetActiveDevice() const { return m_Device; }
    std::vector<GPU::DeviceCaps> ListDevices() const;

    // Buffers | 緩衝
    GPU::ComputeBuffer* CreateBuffer(size_t elementSize, size_t count,
                                     GPU::BufferUsage usage);
    template <typename T>
    GPU::TypedBuffer<T>* CreateTypedBuffer(
        size_t count,
        GPU::BufferUsage usage = GPU::BufferUsage::ReadWrite);

    // Dispatch | 派發
    GPU::CPUBackend::Stats Dispatch(const GPU::Kernel& kernel,
                                    const GPU::DispatchParams& params);

    // Graph | 計算圖
    GPU::ComputeGraph& GetGraph() { return m_Graph; }
    GPU::ComputeGraph::RunResult ExecuteGraph();

    GPU::CPUBackend& GetBackend() { return *m_Backend; }

    // Vulkan backend — initialized lazily on first access when a Vulkan
    // loader + compute device exist; nullptr otherwise. The CPU backend
    // remains the default for lambda kernels (ParallelFor); SPIR-V
    // kernels go through the Vulkan backend.
    // Vulkan 後端 — 首次存取時惰性初始化（有 loader + 計算裝置時），
    // 否則為 nullptr。CPU 後端仍是 lambda kernel 的預設；
    // SPIR-V kernel 走 Vulkan 後端。
    GPU::VulkanBackend* GetVulkanBackend();
    bool HasGPU() { return GetVulkanBackend() != nullptr; }

private:
    GPU::DeviceCaps m_Device;
    std::unique_ptr<GPU::CPUBackend> m_Backend;
    std::unique_ptr<GPU::VulkanBackend> m_Vulkan;
    GPU::ComputeGraph m_Graph;
    std::vector<std::unique_ptr<GPU::ComputeBuffer>> m_Buffers;
    std::vector<std::unique_ptr<void, void(*)(void*)>> m_TypedBuffers;
    bool m_Initialized = false;
};

template <typename T>
GPU::TypedBuffer<T>* IQGPURuntime::CreateTypedBuffer(
    size_t count, GPU::BufferUsage usage) {
    auto* buf = new GPU::TypedBuffer<T>(count, usage);
    m_TypedBuffers.emplace_back(
        buf, [](void* p) { delete static_cast<GPU::TypedBuffer<T>*>(p); });
    return buf;
}

} // namespace IQGPU
} // namespace StonePotatoes
