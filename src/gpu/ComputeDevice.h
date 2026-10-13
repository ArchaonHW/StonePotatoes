#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace GPU {

// Execution device class. Only CPUReference is implemented today;
// GPU/QPU entries declare capability tiers, not availability.
// 執行裝置類別。目前僅實作 CPUReference；
// GPU/QPU 項目僅宣告能力層級，不代表可用性。
enum class DeviceType {
    CPUReference,   // Implemented — multithreaded std::thread dispatch
    DiscreteGPU,    // Planned — Vulkan/D3D12 backend
    IntegratedGPU,  // Planned
    NPU,            // Research
    QPU             // Research — no quantum hardware claimed
};

// Device capabilities queried at runtime.
// 執行期查詢的裝置能力。
struct DeviceCaps {
    DeviceType type = DeviceType::CPUReference;
    std::string name;
    uint32_t computeUnits = 0;      // HW threads / SMs / qubits
    uint64_t memoryBytes = 0;       // usable memory
    bool supportsFloat64 = true;
    bool supportsAtomics = false;
    bool isQuantumCapable = false;  // always false today
};

// Enumerate devices actually present and usable.
// 列舉實際存在且可用的裝置。
std::vector<DeviceCaps> EnumerateDevices();

// Select the best available device for a workload hint.
// 依工作負載提示選擇最佳可用裝置。
DeviceCaps SelectDevice(bool preferGPU = true);

} // namespace GPU
} // namespace StonePotatoes
