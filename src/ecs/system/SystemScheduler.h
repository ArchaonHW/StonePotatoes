#pragma once

#include "System.h"
#include <cstdint>
#include <functional>
#include <vector>
#include <memory>

namespace StonePotatoes {
namespace ECS {

class World;

// Parallel-for executor signature shared with the IQGPU CPU backend
// (CPUBackend::ParallelFor is directly assignable).
// 與 IQGPU CPU 後端共用的 parallel-for 簽名
// （CPUBackend::ParallelFor 可直接指派）。
using ParallelForFn =
    std::function<void(uint32_t, const std::function<void(uint32_t)>&)>;

class SystemScheduler {
public:
    SystemScheduler();
    ~SystemScheduler();

    void RegisterSystem(std::unique_ptr<System> system);

    // Remove a system by pointer. If `world` is non-null the system's
    // Shutdown(world) is called first. Calls made during an update (from
    // inside a System::Update) are deferred until the update finishes, so
    // the running wave is never invalidated. Returns false if the system is
    // not registered (or already pending removal).
    // 依指標移除系統。若 world 非空會先呼叫 Shutdown(world)。在更新期間
    // （System::Update 內）呼叫時延遲至本次更新結束後執行，不會破壞進行中的
    // wave。未註冊（或已排定移除）時回傳 false。
    bool UnregisterSystem(System* system, World* world = nullptr);

    size_t GetSystemCount() const { return m_Systems.size(); }

    void InitializeSystems(World* world);
    void UpdateSystems(World* world, float deltaTime);
    void ShutdownSystems(World* world);

    struct ParallelStats {
        uint32_t waves = 0;        // dependency levels | 依賴層級數
        uint32_t systemsRun = 0;
        uint32_t maxWaveSize = 0;  // widest parallel wave | 最寬平行波
        double ms = 0.0;
    };

    // Wave-scheduled parallel update: systems whose declared accesses
    // don't conflict run in the same wave via parallelFor; ordering
    // between conflicting systems follows registration order.
    // 波次平行更新：宣告存取不衝突的系統經 parallelFor 同波並行；
    // 衝突系統之間維持註冊順序。
    ParallelStats UpdateSystemsParallel(World* world, float deltaTime,
                                        const ParallelForFn& parallelFor);

    // Same wave schedule, run sequentially — for correctness testing.
    // 相同波次排程但循序執行 — 供正確性測試。
    ParallelStats UpdateSystemsWaves(World* world, float deltaTime);

private:
    std::vector<std::vector<size_t>> BuildWaves() const;
    ParallelStats RunWaves(World* world, float deltaTime,
                           const ParallelForFn* parallelFor);

    void RemoveNow(System* system, World* world);
    void FlushPendingRemovals();

    std::vector<std::unique_ptr<System>> m_Systems;

    struct PendingRemoval { System* system; World* world; };
    std::vector<PendingRemoval> m_PendingRemovals;
    bool m_Updating = false;
};

} // namespace ECS
} // namespace StonePotatoes
