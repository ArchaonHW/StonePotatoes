#pragma once

namespace Genesis {
namespace ECS {

class World;

class System {
public:
    virtual ~System() = default;

    virtual void Initialize(World* world) { (void)world; }
    virtual void Update(World* world, float deltaTime) = 0;
    virtual void Shutdown(World* world) { (void)world; }
};

} // namespace ECS
} // namespace Genesis
