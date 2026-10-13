#include "SystemScheduler.h"
#include "../World.h"
#include "../../core/logging/Logger.h"

#include <algorithm>
#include <chrono>

namespace StonePotatoes {
namespace ECS {

SystemScheduler::SystemScheduler() {
}

SystemScheduler::~SystemScheduler() {
}

void SystemScheduler::RegisterSystem(std::unique_ptr<System> system) {
    m_Systems.push_back(std::move(system));
    STONEPOTATOES_LOG_INFO("Registered system, total systems: %zu", m_Systems.size());
}

bool SystemScheduler::UnregisterSystem(System* system, World* world) {
    if (!system) return false;
    const bool registered = std::any_of(
        m_Systems.begin(), m_Systems.end(),
        [&](const std::unique_ptr<System>& s) { return s.get() == system; });
    if (!registered) return false;
    const bool pending = std::any_of(
        m_PendingRemovals.begin(), m_PendingRemovals.end(),
        [&](const PendingRemoval& r) { return r.system == system; });
    if (pending) return false;

    if (m_Updating) {
        // Defer: removing now would invalidate wave indices / iterators.
        // 延遲：此時移除會使 wave 索引／迭代器失效。
        m_PendingRemovals.push_back({system, world});
        return true;
    }
    RemoveNow(system, world);
    return true;
}

void SystemScheduler::RemoveNow(System* system, World* world) {
    auto it = std::find_if(
        m_Systems.begin(), m_Systems.end(),
        [&](const std::unique_ptr<System>& s) { return s.get() == system; });
    if (it == m_Systems.end()) return;
    if (world) (*it)->Shutdown(world);
    // erase keeps relative order → registration-order semantics preserved;
    // waves are rebuilt from m_Systems on every RunWaves call.
    // erase 保留相對順序 → 維持註冊順序語意；每次 RunWaves 皆重建 wave。
    m_Systems.erase(it);
    STONEPOTATOES_LOG_INFO("Unregistered system, total systems: %zu", m_Systems.size());
}

void SystemScheduler::FlushPendingRemovals() {
    auto pending = std::move(m_PendingRemovals);
    m_PendingRemovals.clear();
    for (const auto& r : pending) RemoveNow(r.system, r.world);
}

void SystemScheduler::InitializeSystems(World* world) {
    for (auto& system : m_Systems) {
        system->Initialize(world);
    }
    STONEPOTATOES_LOG_INFO("Initialized %zu systems", m_Systems.size());
}

void SystemScheduler::UpdateSystems(World* world, float deltaTime) {
    m_Updating = true;
    for (auto& system : m_Systems) {
        system->Update(world, deltaTime);
    }
    m_Updating = false;
    FlushPendingRemovals();
}

// ------------------------------------------------------------------
// Wave-scheduled parallel execution | 波次平行排程
// ------------------------------------------------------------------
namespace {

// Two declared accesses conflict if they share a component type that
// at least one of them writes. Undeclared (empty) or structural access
// conflicts with everything — conservative serial barrier.
// 兩份存取宣告衝突的條件：共同組件型別且至少一方寫入。
// 未宣告（空）或結構性存取與一切衝突 — 保守序列屏障。
bool Conflicts(const AccessSpec& a, const AccessSpec& b) {
    if (a.structural || b.structural) return true;
    if (a.entries.empty() || b.entries.empty()) return true;
    for (const auto& ea : a.entries) {
        for (const auto& eb : b.entries) {
            if (ea.type == eb.type && (ea.write || eb.write)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

std::vector<std::vector<size_t>> SystemScheduler::BuildWaves() const {
    std::vector<AccessSpec> specs(m_Systems.size());
    for (size_t i = 0; i < m_Systems.size(); ++i)
        m_Systems[i]->DescribeAccess(specs[i]);

    std::vector<std::vector<size_t>> waves;
    std::vector<size_t> waveOf(m_Systems.size(), 0);
    for (size_t i = 0; i < m_Systems.size(); ++i) {
        // Earliest wave index = 1 + max wave of any conflicting prior
        // system. Members of that wave cannot conflict (else it would
        // be later), so placement is direct.
        // 最早波次 = 衝突前置系統最大波次 + 1。
        size_t minWave = 0;
        for (size_t j = 0; j < i; ++j) {
            if (Conflicts(specs[i], specs[j]) && waveOf[j] + 1 > minWave)
                minWave = waveOf[j] + 1;
        }
        while (waves.size() <= minWave) waves.emplace_back();
        waves[minWave].push_back(i);
        waveOf[i] = minWave;
    }
    return waves;
}

SystemScheduler::ParallelStats
SystemScheduler::RunWaves(World* world, float deltaTime,
                          const ParallelForFn* parallelFor) {
    const auto t0 = std::chrono::steady_clock::now();
    m_Updating = true;
    auto waves = BuildWaves();
    ParallelStats st;
    st.waves = static_cast<uint32_t>(waves.size());
    for (const auto& wave : waves) {
        st.maxWaveSize =
            std::max(st.maxWaveSize, static_cast<uint32_t>(wave.size()));
        if (parallelFor && wave.size() > 1) {
            (*parallelFor)(static_cast<uint32_t>(wave.size()),
                           [&](uint32_t i) {
                               m_Systems[wave[i]]->Update(world,
                                                          deltaTime);
                           });
        } else {
            for (size_t i : wave) m_Systems[i]->Update(world, deltaTime);
        }
        st.systemsRun += static_cast<uint32_t>(wave.size());
        // Deferred commands queued by this wave apply before the next
        // wave starts, so later waves observe the structural results.
        // 本波排入的延遲指令於下一波開始前執行。
        world->ApplyCommands();
    }
    m_Updating = false;
    FlushPendingRemovals();
    st.ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0)
                .count();
    return st;
}

SystemScheduler::ParallelStats
SystemScheduler::UpdateSystemsParallel(World* world, float deltaTime,
                                       const ParallelForFn& parallelFor) {
    return RunWaves(world, deltaTime, &parallelFor);
}

SystemScheduler::ParallelStats
SystemScheduler::UpdateSystemsWaves(World* world, float deltaTime) {
    return RunWaves(world, deltaTime, nullptr);
}

void SystemScheduler::ShutdownSystems(World* world) {
    for (auto& system : m_Systems) {
        system->Shutdown(world);
    }
    STONEPOTATOES_LOG_INFO("Shutdown %zu systems", m_Systems.size());
}

} // namespace ECS
} // namespace StonePotatoes
