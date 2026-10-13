#include "World.h"
#include "../core/logging/Logger.h"

namespace StonePotatoes {
namespace ECS {

World::World()
    : m_EntityManager(std::make_unique<EntityManager>())
    , m_ComponentManager(std::make_unique<ComponentManager>())
    , m_ArchetypeManager(std::make_unique<ArchetypeManager>())
    , m_SystemScheduler(std::make_unique<SystemScheduler>()) {
}

World::~World() {
    Shutdown();
}

void World::Initialize() {
    STONEPOTATOES_LOG_INFO("World: Initializing");
    m_SystemScheduler->InitializeSystems(this);
    STONEPOTATOES_LOG_INFO("World: Initialized");
}

void World::Update(float deltaTime) {
    m_SystemScheduler->UpdateSystems(this, deltaTime);
}

void World::Shutdown() {
    if (m_Shutdown) return;
    m_Shutdown = true;
    STONEPOTATOES_LOG_INFO("World: Shutting down");
    m_SystemScheduler->ShutdownSystems(this);
    STONEPOTATOES_LOG_INFO("World: Shutdown complete");
}

EntityManager& World::GetEntityManager() {
    return *m_EntityManager;
}

ComponentManager& World::GetComponentManager() {
    return *m_ComponentManager;
}

ArchetypeManager& World::GetArchetypeManager() {
    return *m_ArchetypeManager;
}

SystemScheduler& World::GetSystemScheduler() {
    return *m_SystemScheduler;
}

} // namespace ECS
} // namespace StonePotatoes
