#pragma once

#include "ecs/World.h"

#include <array>
#include <string_view>
#include <vector>

namespace StonePotatoes::Editor {

// A fixed-size POD label so editor-created entities survive ECS world saves.
struct EntityLabel {
    std::array<char, 64> value{};
};

// Register before loading a saved editor world so labels can be restored.
void RegisterEditorComponents(ECS::World& world);

// Owns the editor's entity list and selection while editing one ECS world.
class EditorSession {
public:
    explicit EditorSession(ECS::World& world) noexcept;

    ECS::Entity CreateEntity(std::string_view label = {});
    bool SetEntityLabel(ECS::Entity entity, std::string_view label);
    std::string_view GetEntityLabel(ECS::Entity entity) const noexcept;
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
