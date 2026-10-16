#pragma once

#include "ecs/World.h"

#include <vector>

namespace StonePotatoes::Editor {

// Owns the editor's entity list and selection while editing one ECS world.
// The session intentionally exposes identity only; reflection and persistence
// are outside the first editor slice.
class EditorSession {
public:
    explicit EditorSession(ECS::World& world) noexcept;

    ECS::Entity CreateEntity();
    bool SelectEntity(ECS::Entity entity);
    bool DeleteEntity(ECS::Entity entity);

    const std::vector<ECS::Entity>& Entities() const noexcept;
    ECS::Entity SelectedEntity() const noexcept;

private:
    ECS::World& m_World;
    std::vector<ECS::Entity> m_Entities;
    ECS::Entity m_Selected;
};

} // namespace StonePotatoes::Editor
