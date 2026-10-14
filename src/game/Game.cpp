#include "Game.h"

#include "ecs/World.h"

#include <cmath>

namespace StonePotatoes::Game {

bool Game::SetScene(std::unique_ptr<Scene> scene) {
    if (m_Running || !scene) return false;
    m_Scene = std::move(scene);
    return true;
}

bool Game::ChangeScene(std::unique_ptr<Scene> scene) {
    if (!m_Running || !scene) return false;
    m_PendingScene = std::move(scene);
    return true;
}

bool Game::SetFixedStep(float stepSeconds, std::uint32_t maxCatchUpSteps) {
    if (m_Running || !std::isfinite(stepSeconds) || stepSeconds <= 0.0f ||
        maxCatchUpSteps == 0) {
        return false;
    }
    m_FixedStep = stepSeconds;
    m_MaxCatchUpSteps = maxCatchUpSteps;
    m_Accumulator = 0.0;
    return true;
}

bool Game::DisableFixedStep() {
    if (m_Running) return false;
    m_FixedStep = 0.0f;
    m_Accumulator = 0.0;
    return true;
}

bool Game::Start(ECS::World& world) {
    if (m_Running || !m_Scene) return false;
    m_Accumulator = 0.0;
    m_World = &world;
    m_Scene->OnEnter(world);
    m_Running = true;
    return true;
}

void Game::Update(float deltaTime) {
    if (!m_Running || !m_World || !std::isfinite(deltaTime) || deltaTime < 0.0f) return;
    if (m_FixedStep <= 0.0f) {
        UpdateSimulation(deltaTime);
        return;
    }

    m_Accumulator += static_cast<double>(deltaTime);
    const double step = static_cast<double>(m_FixedStep);
    const double epsilon = step * 1e-6;
    std::uint32_t steps = 0;
    while (steps < m_MaxCatchUpSteps && m_Accumulator + epsilon >= step) {
        UpdateSimulation(m_FixedStep);
        m_Accumulator -= step;
        if (m_Accumulator < epsilon) m_Accumulator = 0.0;
        ++steps;
    }

    // Bound per-frame CPU work. Keep only the fractional remainder after the
    // catch-up budget is exhausted, dropping overdue whole simulation ticks.
    if (steps == m_MaxCatchUpSteps && m_Accumulator + epsilon >= step) {
        const double wholeSteps = std::floor((m_Accumulator + epsilon) / step);
        m_Accumulator -= wholeSteps * step;
        if (m_Accumulator < epsilon) m_Accumulator = 0.0;
    }
}

void Game::UpdateSimulation(float deltaTime) {
    if (m_PendingScene) {
        m_Scene->OnExit(*m_World);
        m_Scene = std::move(m_PendingScene);
        m_Scene->OnEnter(*m_World);
    }
    m_Scene->Update(*m_World, deltaTime);
    m_World->Update(deltaTime);
}

void Game::Stop() {
    if (!m_Running || !m_World) return;
    m_Scene->OnExit(*m_World);
    m_PendingScene.reset();
    m_Running = false;
    m_World = nullptr;
    m_Accumulator = 0.0;
}

} // namespace StonePotatoes::Game
