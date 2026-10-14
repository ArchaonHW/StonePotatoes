#pragma once

#include "Scene.h"

#include <cstdint>
#include <memory>

namespace StonePotatoes::ECS {
class World;
}

namespace StonePotatoes::Game {

// Coordinates one active scene with an externally owned ECS world.
class Game {
public:
    bool SetScene(std::unique_ptr<Scene> scene);
    // Scene changes are applied at the beginning of the next Update().
    bool ChangeScene(std::unique_ptr<Scene> scene);
    // Configure fixed simulation ticks before Start(). A frame may execute at
    // most maxCatchUpSteps ticks; excess whole ticks are dropped to bound work.
    bool SetFixedStep(float stepSeconds, std::uint32_t maxCatchUpSteps = 8);
    // Restore one variable-delta update per frame. Only valid while stopped.
    bool DisableFixedStep();
    bool Start(ECS::World& world);
    void Update(float deltaTime);
    void Stop();

    bool IsRunning() const { return m_Running; }
    Scene* GetScene() const { return m_Scene.get(); }

private:
    void UpdateSimulation(float deltaTime);

    std::unique_ptr<Scene> m_Scene;
    std::unique_ptr<Scene> m_PendingScene;
    ECS::World* m_World = nullptr;
    bool m_Running = false;
    float m_FixedStep = 0.0f;
    std::uint32_t m_MaxCatchUpSteps = 8;
    double m_Accumulator = 0.0;
};

} // namespace StonePotatoes::Game
