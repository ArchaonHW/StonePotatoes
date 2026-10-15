#include "VulkanPresentationSmoke.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>

namespace {

void PrintUsage(const char* executable) {
    std::fprintf(stderr,
        "Usage: %s [--frames N] [--resize-at-frame N]\n", executable);
}

bool ParsePositiveInt(const char* value, int& result) {
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (!value[0] || !end || *end != '\0' || parsed <= 0 || parsed > 100000)
        return false;
    result = static_cast<int>(parsed);
    return true;
}

bool ParseNonNegativeInt(const char* value, int& result) {
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (!value[0] || !end || *end != '\0' || parsed < 0 || parsed > 100000)
        return false;
    result = static_cast<int>(parsed);
    return true;
}

} // namespace

int main(int argc, char** argv) {
    int frameLimit = 0;
    int resizeAtFrame = -1;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            if (!ParsePositiveInt(argv[++i], frameLimit)) {
                PrintUsage(argv[0]);
                return 2;
            }
        } else if (std::strcmp(argv[i], "--resize-at-frame") == 0 &&
                   i + 1 < argc) {
            if (!ParseNonNegativeInt(argv[++i], resizeAtFrame)) {
                PrintUsage(argv[0]);
                return 2;
            }
        } else {
            PrintUsage(argv[0]);
            return 2;
        }
    }
    if (resizeAtFrame >= 0 &&
        (frameLimit == 0 || resizeAtFrame >= frameLimit)) {
        PrintUsage(argv[0]);
        return 2;
    }

    StonePotatoes::Rendering::VulkanPresentationSmoke presentation;
    if (!presentation.Initialize()) {
        std::fprintf(stderr, "Vulkan presentation initialization failed: %s\n",
                     presentation.LastError().c_str());
        return 1;
    }

    bool resizeVerified = resizeAtFrame < 0;
    for (int frame = 0;
         !presentation.ShouldClose() &&
         (frameLimit == 0 || frame < frameLimit);
         ++frame) {
        presentation.PollEvents();
        if (frame == resizeAtFrame) {
            GLFWwindow* window = presentation.GetWindow();
            const std::uint64_t oldGeneration =
                presentation.GetSwapchainGeneration();
            if (!window) {
                std::fprintf(stderr, "Vulkan window is unavailable\n");
                presentation.Shutdown();
                return 1;
            }
            int width = 0;
            int height = 0;
            glfwGetWindowSize(window, &width, &height);
            if (width <= 0 || height <= 0) {
                std::fprintf(stderr, "Vulkan window has invalid size\n");
                presentation.Shutdown();
                return 1;
            }
            glfwSetWindowSize(window, width + 128, height + 96);

            // The event queue may deliver the framebuffer resize on a later
            // poll, so keep presenting briefly until the helper rebuilds its
            // swapchain or the bounded smoke window expires.
            for (int attempt = 0;
                 attempt < 60 &&
                 presentation.GetSwapchainGeneration() == oldGeneration;
                 ++attempt) {
                glfwWaitEventsTimeout(0.05);
                presentation.PollEvents();
                if (!presentation.PresentClearFrame()) {
                    std::fprintf(stderr, "Vulkan resize frame failed: %s\n",
                                 presentation.LastError().c_str());
                    presentation.Shutdown();
                    return 1;
                }
            }
            if (presentation.GetSwapchainGeneration() == oldGeneration) {
                std::fprintf(stderr,
                    "Vulkan swapchain was not recreated after resize\n");
                presentation.Shutdown();
                return 1;
            }
            if (!presentation.PresentClearFrame()) {
                std::fprintf(stderr,
                    "Vulkan frame after swapchain recreation failed: %s\n",
                    presentation.LastError().c_str());
                presentation.Shutdown();
                return 1;
            }
            resizeVerified = true;
        }
        if (!presentation.PresentClearFrame()) {
            std::fprintf(stderr, "Vulkan frame failed: %s\n",
                         presentation.LastError().c_str());
            presentation.Shutdown();
            return 1;
        }
    }
    if (!resizeVerified) {
        std::fprintf(stderr, "Vulkan resize verification did not run\n");
        presentation.Shutdown();
        return 1;
    }
    presentation.Shutdown();
    return 0;
}
