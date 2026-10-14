#pragma once

#include "ComputeDevice.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// VulkanBackend — REAL Vulkan compute backend for IQGPU.
// Scope (honest): synchronous dispatches on a single compute queue,
// host-visible coherent buffers only (no discrete-VRAM staging yet),
// GLSL→SPIR-V kernels compiled at build time by glslc. Multi-queue,
// async uploads, and memory barriers remain Planned.
// VulkanBackend — IQGPU 的真實 Vulkan 計算後端。
// 範圍（誠實）：單一計算佇列的同步派發、僅 host-visible coherent
// 緩衝（尚無獨立 VRAM staging）、GLSL→SPIR-V 由 glslc 建置期編譯。
// 多佇列、非同步上傳與記憶體屏障仍為計畫中。
namespace StonePotatoes {
namespace GPU {

class VulkanContext;

class VulkanBackend {
public:
    // Opaque handles (index into internal tables, never a Vk* pointer).
    // 不透明控制代碼（內部表索引，絕非 Vk* 指標）。
    struct BufferHandle { uint64_t id = ~0ull; bool Valid() const { return id != ~0ull; } };
    struct KernelHandle { uint64_t id = ~0ull; bool Valid() const { return id != ~0ull; } };

    // True when a Vulkan loader + a compute-capable device exist.
    // 當 Vulkan loader 與具計算能力的裝置存在時為真。
    static bool IsAvailable();

    // Enumerate Vulkan physical devices as DeviceCaps (no instance kept).
    // 以 DeviceCaps 列舉 Vulkan 實體裝置（不保留 instance）。
    static std::vector<DeviceCaps> EnumerateGPUs();

    explicit VulkanBackend(std::shared_ptr<VulkanContext> context = {});
    ~VulkanBackend();
    VulkanBackend(const VulkanBackend&) = delete;
    VulkanBackend& operator=(const VulkanBackend&) = delete;

    // deviceIndex: index into EnumerateGPUs() order. 裝置索引。
    bool Initialize(uint32_t deviceIndex = 0);
    void Shutdown();
    bool IsInitialized() const;
    // Shared owner used by internal graphics consumers. The context remains
    // alive until the backend and all consumers release their shared handles.
    std::shared_ptr<VulkanContext> GetContext() const;
    const DeviceCaps& GetCaps() const;
    const std::string& LastError() const;

    // Host-visible coherent storage buffer. 主機可見一致儲存緩衝。
    BufferHandle CreateBuffer(uint64_t bytes);
    void DestroyBuffer(BufferHandle h);
    bool WriteBuffer(BufferHandle h, const void* data, uint64_t bytes,
                     uint64_t offset = 0);
    bool ReadBuffer(BufferHandle h, void* out, uint64_t bytes,
                    uint64_t offset = 0) const;

    // Compute kernel from SPIR-V words; bufferCount = #storage buffers in
    // descriptor set 0; pushConstantBytes ≤ 128 optional.
    // 由 SPIR-V 建立計算 kernel；bufferCount 為 set 0 的儲存緩衝數。
    KernelHandle CreateKernel(const uint32_t* spirv, size_t spirvBytes,
                              uint32_t bufferCount,
                              uint32_t pushConstantBytes = 0);
    KernelHandle CreateKernelFromFile(const std::string& spvPath,
                                      uint32_t bufferCount,
                                      uint32_t pushConstantBytes = 0);
    void DestroyKernel(KernelHandle h);

    struct DispatchResult {
        bool ok = false;
        double gpuMs = 0.0;   // timestamp-query GPU time; =wallMs if unsupported
        double wallMs = 0.0;  // submit + fence wait | 提交 + fence 等待
    };

    // Synchronous dispatch: binds buffers in order to set 0, optional push
    // constants, waits on a fence. 同步派發：依序綁定緩衝至 set 0，
    // 可選 push constants，以 fence 等待完成。
    DispatchResult Dispatch(KernelHandle kernel,
                            const std::vector<BufferHandle>& buffers,
                            uint32_t groupsX, uint32_t groupsY = 1,
                            uint32_t groupsZ = 1,
                            const void* pushConstants = nullptr,
                            uint32_t pushConstantBytes = 0);

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // namespace GPU
} // namespace StonePotatoes
