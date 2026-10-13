#include "SystemScheduler.h"
#include "../World.h"
#include "../../core/logging/Logger.h"

namespace StonePotatoes {
namespace ECS {

SystemScheduler::SystemScheduler() {
}

SystemScheduler::~SystemScheduler() {
}

void SystemScheduler::RegisterSystem(std::unique_ptr<System> system) {
    (void)system; // Suppress unused parameter warning
    m_Systems.push_back(std::move(system));
    STONEPOTATOES_LOG_INFO("Registered system, total systems: %zu", m_Systems.size());
}

void SystemScheduler::UnregisterSystem(System* system) {
    // TODO: Implement system unregistration
    STONEPOTATOES_LOG_WARNING("SystemScheduler::UnregisterSystem: TODO - Implement system unregistration");
}

void SystemScheduler::InitializeSystems(World* world) {
    for (auto& system : m_Systems) {
        system->Initialize(world);
    }
    STONEPOTATOES_LOG_INFO("Initialized %zu systems", m_Systems.size());
}

void SystemScheduler::UpdateSystems(World* world, float deltaTime) {
    for (auto& system : m_Systems) {
        system->Update(world, deltaTime);
    }
}

void SystemScheduler::ShutdownSystems(World* world) {
    for (auto& system : m_Systems) {
        system->Shutdown(world);
    }
    STONEPOTATOES_LOG_INFO("Shutdown %zu systems", m_Systems.size());
}

} // namespace ECS
} // namespace StonePotatoes
