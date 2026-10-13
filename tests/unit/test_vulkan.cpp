// StonePotatoes Vulkan backend tests — real GPU dispatch on real hardware.
// Skips cleanly when no Vulkan loader/device exists (CI without GPU).
// StonePotatoes Vulkan 後端測試 — 在真實硬體上做真實 GPU 派發。
// 無 Vulkan loader/裝置時乾淨跳過（無 GPU 的 CI）。

#include "iqgpu/IQGPURuntime.h"
#include "gpu/VulkanBackend.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace StonePotatoes;

static int g_Passed = 0;
static int g_Failed = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (cond) { ++g_Passed; }                                          \
        else {                                                             \
            ++g_Failed;                                                    \
            std::printf("[FAIL] %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
        }                                                                  \
    } while (0)

#ifndef SP_SHADER_DIR
#define SP_SHADER_DIR "shaders"
#endif

static const char* kSaxpy = SP_SHADER_DIR "/saxpy.spv";
static const char* kU32Add = SP_SHADER_DIR "/u32add.spv";

void TestEnumerate() {
    const auto gpus = GPU::VulkanBackend::EnumerateGPUs();
    CHECK(!gpus.empty());
    for (const auto& g : gpus) {
        std::printf("[VK] %s  type=%s  mem=%.1f GiB  f64=%d\n", g.name.c_str(),
                    g.type == GPU::DeviceType::DiscreteGPU ? "dGPU" : "iGPU",
                    g.memoryBytes / 1073741824.0, g.supportsFloat64 ? 1 : 0);
        CHECK(!g.name.empty());
        CHECK(g.memoryBytes > 0);
    }
}

// u32 add: integer math must be BIT-IDENTICAL CPU vs GPU.
// u32 加法：整數運算 CPU 與 GPU 必須位元一致。
void TestU32BitExact(GPU::VulkanBackend& vk) {
    constexpr uint32_t N = 1u << 20;
    std::vector<uint32_t> a(N), b(N), c(N, 0xDEADBEEF);
    for (uint32_t i = 0; i < N; ++i) {
        a[i] = i * 2654435761u;
        b[i] = i ^ 0x9E3779B9u;
    }
    auto ha = vk.CreateBuffer(N * 4), hb = vk.CreateBuffer(N * 4),
         hc = vk.CreateBuffer(N * 4);
    CHECK(ha.Valid() && hb.Valid() && hc.Valid());
    CHECK(vk.WriteBuffer(ha, a.data(), N * 4));
    CHECK(vk.WriteBuffer(hb, b.data(), N * 4));

    auto kernel = vk.CreateKernelFromFile(kU32Add, 3, sizeof(uint32_t));
    CHECK(kernel.Valid());
    const uint32_t n = N;
    auto r = vk.Dispatch(kernel, {ha, hb, hc}, (N + 255) / 256, 1, 1, &n,
                         sizeof(n));
    CHECK(r.ok);
    std::printf("[VK] u32add %u items: gpu=%.3fms wall=%.3fms\n", N, r.gpuMs,
                r.wallMs);

    CHECK(vk.ReadBuffer(hc, c.data(), N * 4));
    bool exact = true;
    for (uint32_t i = 0; i < N && exact; ++i)
        exact = c[i] == a[i] + b[i];
    CHECK(exact);  // bit-identical — the determinism contract | 位元一致

    vk.DestroyKernel(kernel);
    vk.DestroyBuffer(ha);
    vk.DestroyBuffer(hb);
    vk.DestroyBuffer(hc);
}

// saxpy float: GPU may fuse to FMA while CPU does mul+add — allow 1 ulp-class
// tolerance, verify near-zero error overall.
// saxpy 浮點：GPU 可能融合為 FMA，容許極小誤差。
void TestSaxpy(GPU::VulkanBackend& vk) {
    constexpr uint32_t N = 1u << 22;
    std::vector<float> a(N), b(N), c(N);
    for (uint32_t i = 0; i < N; ++i) {
        a[i] = static_cast<float>(i % 1024) * 0.5f;
        b[i] = static_cast<float>(i % 512) * 0.25f;
    }
    auto ha = vk.CreateBuffer(N * 4), hb = vk.CreateBuffer(N * 4),
         hc = vk.CreateBuffer(N * 4);
    CHECK(vk.WriteBuffer(ha, a.data(), N * 4));
    CHECK(vk.WriteBuffer(hb, b.data(), N * 4));

    auto kernel = vk.CreateKernelFromFile(kSaxpy, 3, 8);
    CHECK(kernel.Valid());
    struct { float alpha; uint32_t n; } pc{2.5f, N};
    auto r = vk.Dispatch(kernel, {ha, hb, hc}, (N + 255) / 256, 1, 1, &pc,
                         sizeof(pc));
    CHECK(r.ok);
    std::printf("[VK] saxpy %u floats: gpu=%.3fms wall=%.3fms "
                "(%.1f GB/s effective)\n", N, r.gpuMs, r.wallMs,
                3.0 * N * 4.0 / (r.gpuMs * 1e6));

    CHECK(vk.ReadBuffer(hc, c.data(), N * 4));
    double maxErr = 0.0;
    for (uint32_t i = 0; i < N; ++i)
        maxErr = std::max(maxErr, static_cast<double>(
                          std::fabs(c[i] - (2.5f * a[i] + b[i]))));
    CHECK(maxErr < 1e-3);

    vk.DestroyKernel(kernel);
    vk.DestroyBuffer(ha);
    vk.DestroyBuffer(hb);
    vk.DestroyBuffer(hc);
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);  // survive crashes | 當機時仍能輸出
    std::printf("=== StonePotatoes Vulkan Backend Tests ===\n");
    if (!GPU::VulkanBackend::IsAvailable()) {
        std::printf("Vulkan unavailable — SKIP (not a failure)\n");
        return 0;
    }
    TestEnumerate();

    GPU::VulkanBackend vk;
    CHECK(vk.Initialize());
    std::printf("[VK] active device: %s (f64=%d)\n", vk.GetCaps().name.c_str(),
                vk.GetCaps().supportsFloat64 ? 1 : 0);
    if (!vk.IsInitialized()) {
        std::printf("init failed: %s\n", vk.LastError().c_str());
        std::printf("=== %d passed, %d failed ===\n", g_Passed, g_Failed);
        return 1;
    }

    TestU32BitExact(vk);
    TestSaxpy(vk);
    vk.Shutdown();

    std::printf("=== %d passed, %d failed ===\n", g_Passed, g_Failed);
    return g_Failed == 0 ? 0 : 1;
}
