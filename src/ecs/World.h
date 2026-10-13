#pragma once

#include "entity/EntityManager.h"
#include "component/ComponentManager.h"
#include "archetype/ArchetypeManager.h"
#include "system/SystemScheduler.h"
#include <memory>

namespace StonePotatoes {
namespace ECS {

class World {
public:
    World();
    ~World();

    void Initialize();
    void Update(float deltaTime);
    void Shutdown();

    EntityManager& GetEntityManager();
    ComponentManager& GetComponentManager();
    ArchetypeManager& GetArchetypeManager();
    SystemScheduler& GetSystemScheduler();

private:
    std::unique_ptr<EntityManager> m_EntityManager;
    std::unique_ptr<ComponentManager> m_ComponentManager;
    std::unique_ptr<ArchetypeManager> m_ArchetypeManager;
    std::unique_ptr<SystemScheduler> m_SystemScheduler;
    bool m_Shutdown = false;
};

} // namespace ECS
} // namespace StonePotatoes
