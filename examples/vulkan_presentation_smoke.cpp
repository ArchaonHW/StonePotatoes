#include "VulkanPresentationSmoke.h"

#include <cstdio>

int main() {
    StonePotatoes::Rendering::VulkanPresentationSmoke presentation;
    if (!presentation.Initialize()) {
        std::fprintf(stderr, "Vulkan presentation initialization failed: %s\n",
                     presentation.LastError().c_str());
        return 1;
    }

    while (!presentation.ShouldClose()) {
        presentation.PollEvents();
        if (!presentation.PresentClearFrame()) {
            std::fprintf(stderr, "Vulkan frame failed: %s\n",
                         presentation.LastError().c_str());
            presentation.Shutdown();
            return 1;
        }
    }
    presentation.Shutdown();
    return 0;
}
