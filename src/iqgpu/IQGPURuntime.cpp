#include "IQGPURuntime.h"
#include "../core/logging/Logger.h"

namespace StonePotatoes {
namespace IQGPU {

IQGPURuntime::IQGPURuntime() = default;
IQGPURuntime::~IQGPURuntime() = default;

void IQGPURuntime::Initialize() {
    if (m_Initialized) return;
    m_Device = GPU::SelectDevice(/*preferGPU=*/true);
    // CPU backend workers come from hardware concurrency — NOT from the
    // selected device caps (a GPU caps entry would give queueCount=1).
    // CPU 後端 worker 數取決於硬體並行度 — 非選定裝置的 caps
    // （GPU caps 會回傳 queueCount=1）。
    m_Backend = std::make_unique<GPU::CPUBackend>();
    m_Initialized = true;
    STONEPOTATOES_LOG_INFO(
        "[IQGPU] Initialized | 已初始化 — device=%s units=%u",
        m_Device.name.c_str(), m_Device.computeUnits);
    STONEPOTATOES_LOG_INFO(
        "[IQGPU] Backend: CPU reference | 後端：CPU 參考實作"
        "%s", GPU::VulkanBackend::IsAvailable()
                  ? " + Vulkan (lazy) | + Vulkan（惰性）"
                  : " (GPU backend: Planned | GPU 後端：計畫中)");
}

void IQGPURuntime::Shutdown() {
    if (!m_Initialized) return;
    m_Graph.Clear();
    m_Buffers.clear();
    m_TypedBuffers.clear();
    m_Vulkan.reset();
    m_Backend.reset();
    m_Initialized = false;
    STONEPOTATOES_LOG_INFO("[IQGPU] Shutdown | 關閉");
}

std::vector<GPU::DeviceCaps> IQGPURuntime::ListDevices() const {
    return GPU::EnumerateDevices();
}

GPU::ComputeBuffer* IQGPURuntime::CreateBuffer(size_t elementSize,
                                               size_t count,
                                               GPU::BufferUsage usage) {
    auto buf = std::make_unique<GPU::ComputeBuffer>(elementSize, count, usage);
    auto* raw = buf.get();
    m_Buffers.push_back(std::move(buf));
    return raw;
}

GPU::CPUBackend::Stats
IQGPURuntime::Dispatch(const GPU::Kernel& kernel,
                       const GPU::DispatchParams& params) {
    return m_Backend->Dispatch(kernel, params);
}

GPU::ComputeGraph::RunResult IQGPURuntime::ExecuteGraph() {
    return m_Graph.Execute(*m_Backend);
}

GPU::VulkanBackend* IQGPURuntime::GetVulkanBackend() {
    if (!m_Initialized) Initialize();
    if (!m_Vulkan && GPU::VulkanBackend::IsAvailable()) {
        auto vk = std::make_unique<GPU::VulkanBackend>();
        if (vk->Initialize()) {
            STONEPOTATOES_LOG_INFO(
                "[IQGPU] Vulkan backend up | Vulkan 後端就緒 — %s",
                vk->GetCaps().name.c_str());
            m_Vulkan = std::move(vk);
        } else {
            STONEPOTATOES_LOG_INFO(
                "[IQGPU] Vulkan init failed | Vulkan 初始化失敗 — %s",
                vk->LastError().c_str());
        }
    }
    return m_Vulkan.get();
}

} // namespace IQGPU
} // namespace StonePotatoes
