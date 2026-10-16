#include "EditorSession.h"

#include <algorithm>

namespace StonePotatoes::Editor {

EditorSession::EditorSession(ECS::World& world) noexcept : m_World(world) {}

ECS::Entity EditorSession::CreateEntity() {
    const ECS::Entity entity = m_World.CreateEntity();
    m_Entities.push_back(entity);
    m_Selected = entity;
    return entity;
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
