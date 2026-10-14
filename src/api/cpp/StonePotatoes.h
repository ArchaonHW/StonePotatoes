#pragma once

#include "ecs/World.h"
#include "game/Game.h"

#include <cstdint>
#include <memory>
#include <utility>

namespace StonePotatoes::API {

// Headless application entry point. Configure the ECS world, set a scene,
// then call Start/Update/Stop from the host application's loop.
class Runtime final {
public:
    Runtime() = default;
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&&) = delete;
    Runtime& operator=(Runtime&&) = delete;

    ~Runtime() { Stop(); }

    ECS::World& GetWorld() { return m_World; }
    Game::Game& GetGame() { return m_Game; }

    bool SetScene(std::unique_ptr<Game::Scene> scene) {
        return m_Game.SetScene(std::move(scene));
    }

    bool SetFixedStep(float stepSeconds, std::uint32_t maxCatchUpSteps = 8) {
        return m_Game.SetFixedStep(stepSeconds, maxCatchUpSteps);
    }

    bool DisableFixedStep() { return m_Game.DisableFixedStep(); }

    bool Start() {
        if (m_Game.IsRunning() || !m_Game.GetScene()) return false;
        if (!m_Initialized) {
            m_World.Initialize();
            m_Initialized = true;
        }
        return m_Game.Start(m_World);
    }

    void Update(float deltaTime) { m_Game.Update(deltaTime); }
    void Stop() { m_Game.Stop(); }

    bool IsRunning() const { return m_Game.IsRunning(); }

private:
    // Destruction runs in reverse declaration order, so Game stops before
    // World performs its shutdown in the World destructor.
    ECS::World m_World;
    Game::Game m_Game;
    bool m_Initialized = false;
};

} // namespace StonePotatoes::API
