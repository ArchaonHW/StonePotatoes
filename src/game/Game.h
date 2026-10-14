#pragma once

#include "Scene.h"

#include <memory>

namespace StonePotatoes::ECS {
class World;
}

namespace StonePotatoes::Game {

// Coordinates one active scene with an externally owned ECS world.
class Game {
public:
    bool SetScene(std::unique_ptr<Scene> scene);
    bool Start(ECS::World& world);
    void Update(float deltaTime);
    void Stop();

    bool IsRunning() const { return m_Running; }
    Scene* GetScene() const { return m_Scene.get(); }

private:
    std::unique_ptr<Scene> m_Scene;
    ECS::World* m_World = nullptr;
    bool m_Running = false;
};

} // namespace StonePotatoes::Game
