#include "IQGPURuntime.h"
#include "../core/logging/Logger.h"

namespace StonePotatoes {
namespace IQGPU {

IQGPURuntime::IQGPURuntime() = default;
IQGPURuntime::~IQGPURuntime() = default;

void IQGPURuntime::Initialize() {
    if (m_Initialized) return;
    m_Device = GPU::SelectDevice(/*preferGPU=*/true);
    m_Backend = std::make_unique<GPU::CPUBackend>(m_Device.computeUnits);
    m_Initialized = true;
    STONEPOTATOES_LOG_INFO(
        "[IQGPU] Initialized | 已初始化 — device=%s units=%u",
        m_Device.name.c_str(), m_Device.computeUnits);
    STONEPOTATOES_LOG_INFO(
        "[IQGPU] Backend: CPU reference | 後端：CPU 參考實作"
        " (GPU backend: Planned | GPU 後端：計畫中)");
}

void IQGPURuntime::Shutdown() {
    if (!m_Initialized) return;
    m_Graph.Clear();
    m_Buffers.clear();
    m_TypedBuffers.clear();
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

} // namespace IQGPU
} // namespace StonePotatoes
