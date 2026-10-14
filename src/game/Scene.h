#pragma once

namespace StonePotatoes::ECS {
class World;
}

namespace StonePotatoes::Game {

class Scene {
public:
    virtual ~Scene() = default;
    virtual void OnEnter(ECS::World& world) { (void)world; }
    virtual void Update(ECS::World& world, float deltaTime) {
        (void)world;
        (void)deltaTime;
    }
    virtual void OnExit(ECS::World& world) { (void)world; }
};

} // namespace StonePotatoes::Game
