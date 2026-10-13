#pragma once

#include "System.h"
#include <vector>
#include <memory>

namespace Genesis {
namespace ECS {

class World;

class SystemScheduler {
public:
    SystemScheduler();
    ~SystemScheduler();

    void RegisterSystem(std::unique_ptr<System> system);
    void UnregisterSystem(System* system); // NOT IMPLEMENTED

    void InitializeSystems(World* world);
    void UpdateSystems(World* world, float deltaTime);
    void ShutdownSystems(World* world);

private:
    std::vector<std::unique_ptr<System>> m_Systems;
};

} // namespace ECS
} // namespace Genesis
