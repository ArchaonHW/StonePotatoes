#include "core/platform/Platform.h"
#include "gpu/VulkanBackend.h"
#include "gpu/VulkanContext.h"
#include "rendering/Renderer.h"

#include <imgui.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <memory>

namespace {

constexpr uint32_t kElementCount = 64;

struct PushConstants {
    float alpha;
    uint32_t count;
};
static_assert(sizeof(PushConstants) == 8);

int RunDemo() {
    using namespace StonePotatoes;

    auto context = std::make_shared<GPU::VulkanContext>();
    Rendering::Renderer renderer(context);
    if (!renderer.Initialize({1024, 640, "StonePotatoes Vulkan Compute + Renderer"})) {
        std::fprintf(stderr, "Renderer initialization failed\n");
        return 1;
    }

    GPU::VulkanBackend compute(context);
    if (!compute.Initialize()) {
        std::fprintf(stderr, "Vulkan compute initialization failed: %s\n",
                     compute.LastError().c_str());
        return 1;
    }

    const auto renderContext = renderer.GetVulkanContext();
    const auto computeContext = compute.GetContext();
    if (!renderContext || !computeContext ||
        renderContext.get() != computeContext.get() ||
        renderContext->GetDevice() != computeContext->GetDevice()) {
        std::fprintf(stderr, "Renderer and compute do not share one VkDevice\n");
        return 1;
    }

    std::array<float, kElementCount> a{};
    std::array<float, kElementCount> b{};
    std::array<float, kElementCount> output{};
    for (uint32_t i = 0; i < kElementCount; ++i) {
        a[i] = static_cast<float>(i) * 0.1f;
        b[i] = 2.0f;
    }

    const auto bufferA = compute.CreateBuffer(sizeof(a));
    const auto bufferB = compute.CreateBuffer(sizeof(b));
    const auto bufferOutput = compute.CreateBuffer(sizeof(output));
    const auto kernel = compute.CreateKernelFromFile(
        SP_SHADER_DIR "/saxpy.spv", 3, sizeof(PushConstants));
    if (!bufferA.Valid() || !bufferB.Valid() || !bufferOutput.Valid() ||
        !kernel.Valid()) {
        std::fprintf(stderr, "Failed to create Vulkan compute resources: %s\n",
                     compute.LastError().c_str());
        return 1;
    }

    if (!compute.WriteBuffer(bufferA, a.data(), sizeof(a)) ||
        !compute.WriteBuffer(bufferB, b.data(), sizeof(b))) {
        std::fprintf(stderr, "Failed to upload compute inputs: %s\n",
                     compute.LastError().c_str());
        return 1;
    }
    const PushConstants constants{3.0f, kElementCount};
    const auto dispatch = compute.Dispatch(
        kernel, {bufferA, bufferB, bufferOutput}, 1, 1, 1, &constants,
        sizeof(constants));
    if (!dispatch.ok ||
        !compute.ReadBuffer(bufferOutput, output.data(), sizeof(output))) {
        std::fprintf(stderr, "Vulkan compute dispatch/read failed: %s\n",
                     compute.LastError().c_str());
        return 1;
    }
    for (uint32_t i = 0; i < kElementCount; ++i) {
        const float expected = constants.alpha * a[i] + b[i];
        if (std::abs(output[i] - expected) > 1.0e-5f) {
            std::fprintf(stderr,
                         "Compute result mismatch at %u: got %.6f, expected %.6f\n",
                         i, output[i], expected);
            return 1;
        }
    }

    std::printf("Shared VkDevice verified; SAXPY validated %u values (%.3f ms GPU, %.3f ms wall).\n",
                kElementCount, dispatch.gpuMs, dispatch.wallMs);
    while (!renderer.ShouldClose()) {
        renderer.BeginFrame();
        for (uint32_t i = 0; i < kElementCount; ++i) {
            const float x = static_cast<float>(i % 8) * 4.0f - 14.0f;
            const float y = static_cast<float>(i / 8) * 3.0f - 10.0f;
            const float intensity = output[i] / 22.0f;
            renderer.DrawPoint(x, y, 0.45f,
                               {0.15f, intensity, 1.0f - intensity * 0.6f});
        }
        renderer.BeginImGui();
        ImGui::Text("Device: %s", compute.GetCaps().name.c_str());
        ImGui::Text("Shared VkDevice: verified");
        ImGui::Text("SAXPY values checked: %u", kElementCount);
        ImGui::Text("GPU: %.3f ms | wall: %.3f ms", dispatch.gpuMs,
                    dispatch.wallMs);
        ImGui::Text("Close the window to exit.");
        renderer.EndImGui();
        renderer.EndFrame();
    }

    compute.Shutdown();
    renderer.Shutdown();
    return 0;
}

} // namespace

int main() {
    if (!StonePotatoes::Platform::Initialize()) {
        std::fprintf(stderr, "Platform initialization failed\n");
        return 1;
    }
    const int result = RunDemo();
    StonePotatoes::Platform::Shutdown();
    return result;
}
