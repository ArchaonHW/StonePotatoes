#include "ComputeDevice.h"
#include "VulkanBackend.h"
#include <thread>
#include <cstdlib>

#ifdef _WIN32
#include <malloc.h>
#endif

namespace StonePotatoes {
namespace GPU {

std::vector<DeviceCaps> EnumerateDevices() {
    std::vector<DeviceCaps> devices;

    DeviceCaps cpu;
    cpu.type = DeviceType::CPUReference;
    cpu.name = "CPU Reference Backend";
    cpu.computeUnits = std::thread::hardware_concurrency();
    if (cpu.computeUnits == 0) cpu.computeUnits = 1;

    // Best-effort physical memory query (portable subset).
    // 可攜式的實體記憶體概略查詢。
    cpu.memoryBytes = 0; // unknown via std::; platform query is Planned
    cpu.supportsFloat64 = true;
    cpu.supportsAtomics = true;
    devices.push_back(cpu);

    // Vulkan GPUs when a loader + compute device exist (QPU: still none).
    // 有 Vulkan loader 與計算裝置時列入 GPU（QPU：仍無）。
    auto gpus = VulkanBackend::EnumerateGPUs();
    devices.insert(devices.end(), gpus.begin(), gpus.end());
    return devices;
}

DeviceCaps SelectDevice(bool preferGPU) {
    auto devices = EnumerateDevices();
    if (preferGPU) {
        for (const auto& d : devices) {
            if (d.type == DeviceType::DiscreteGPU ||
                d.type == DeviceType::IntegratedGPU) {
                return d;
            }
        }
    }
    // Fall back to the CPU reference device (always present).
    // 退回 CPU 參考裝置（必定存在）。
    return devices.front();
}

} // namespace GPU
} // namespace StonePotatoes
