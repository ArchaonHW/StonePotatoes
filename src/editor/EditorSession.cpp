#include "EditorSession.h"

#include <algorithm>
#include <cstring>
#include <string>

namespace StonePotatoes::Editor {

void RegisterEditorComponents(ECS::World& world) {
    world.RegisterComponent<EntityLabel>("editor.entity-label.v1");
}

EditorSession::EditorSession(ECS::World& world) noexcept : m_World(world) {
    m_World.GetEntityManager().ForEachAlive(
        [this](ECS::Entity entity) { m_Entities.push_back(entity); });
}

ECS::Entity EditorSession::CreateEntity(std::string_view label) {
    const ECS::Entity entity = m_World.CreateEntity();
    m_Entities.push_back(entity);
    m_Selected = entity;
    if (label.empty()) {
        const std::string defaultLabel =
            "Entity " + std::to_string(entity.GetIndex());
        SetEntityLabel(entity, defaultLabel);
    } else {
        SetEntityLabel(entity, label);
    }
    return entity;
}

bool EditorSession::SetEntityLabel(ECS::Entity entity,
                                  std::string_view label) {
    if (!m_World.IsEntityValid(entity) || label.empty() ||
        label.size() >= EntityLabel{}.value.size() ||
        std::find(m_Entities.begin(), m_Entities.end(), entity) ==
            m_Entities.end()) {
        return false;
    }

    EntityLabel* component = m_World.GetComponent<EntityLabel>(entity);
    if (!component) {
        m_World.AddComponent(entity, EntityLabel{});
        component = m_World.GetComponent<EntityLabel>(entity);
    }
    if (!component) return false;

    component->value.fill('\0');
    std::memcpy(component->value.data(), label.data(), label.size());
    return true;
}

std::string_view
EditorSession::GetEntityLabel(ECS::Entity entity) const noexcept {
    const EntityLabel* component = m_World.GetComponent<EntityLabel>(entity);
    if (!component) return {};
    const auto end = std::find(component->value.begin(),
                               component->value.end(), '\0');
    return {component->value.data(),
            static_cast<size_t>(end - component->value.begin())};
}

bool EditorSession::SelectEntity(ECS::Entity entity) {
    if (!m_World.IsEntityValid(entity) ||
        std::find(m_Entities.begin(), m_Entities.end(), entity) ==
            m_Entities.end()) {
        return false;
    }
    m_Selected = entity;
    return true;
}

bool EditorSession::DeleteEntity(ECS::Entity entity) {
    const auto found = std::find(m_Entities.begin(), m_Entities.end(), entity);
    if (found == m_Entities.end() || !m_World.IsEntityValid(entity)) {
        return false;
    }

    m_World.DestroyEntity(entity);
    m_Entities.erase(found);
    if (m_Selected == entity) {
        m_Selected = ECS::INVALID_ENTITY;
    }
    return true;
}

const std::vector<ECS::Entity>& EditorSession::Entities() const noexcept {
    return m_Entities;
}

ECS::Entity EditorSession::SelectedEntity() const noexcept {
    return m_Selected;
}

} // namespace StonePotatoes::Editor
