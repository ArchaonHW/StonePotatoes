#include "Game.h"

#include "ecs/World.h"

namespace StonePotatoes::Game {

bool Game::SetScene(std::unique_ptr<Scene> scene) {
    if (m_Running || !scene) return false;
    m_Scene = std::move(scene);
    return true;
}

bool Game::Start(ECS::World& world) {
    if (m_Running || !m_Scene) return false;
    m_World = &world;
    m_Scene->OnEnter(world);
    m_Running = true;
    return true;
}

void Game::Update(float deltaTime) {
    if (!m_Running || !m_World) return;
    m_Scene->Update(*m_World, deltaTime);
    m_World->Update(deltaTime);
}

void Game::Stop() {
    if (!m_Running || !m_World) return;
    m_Scene->OnExit(*m_World);
    m_Running = false;
    m_World = nullptr;
}

} // namespace StonePotatoes::Game
