#include "World.h"
#include "../core/logging/Logger.h"

namespace Genesis {
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
    GENESIS_LOG_INFO("World: Initializing");
    m_SystemScheduler->InitializeSystems(this);
    GENESIS_LOG_INFO("World: Initialized");
}

void World::Update(float deltaTime) {
    m_SystemScheduler->UpdateSystems(this, deltaTime);
}

void World::Shutdown() {
    GENESIS_LOG_INFO("World: Shutting down");
    m_SystemScheduler->ShutdownSystems(this);
    GENESIS_LOG_INFO("World: Shutdown complete");
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
} // namespace Genesis
