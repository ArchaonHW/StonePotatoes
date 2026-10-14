#include "EntityManager.h"
#include "../../core/logging/Logger.h"

#include <limits>

namespace StonePotatoes {
namespace ECS {

EntityManager::EntityManager() {
    m_Generations.reserve(1024);
    m_Alive.reserve(1024);
}

EntityManager::~EntityManager() = default;

Entity EntityManager::CreateEntity() {
    uint32_t index;
    if (!m_FreeList.empty()) {
        index = m_FreeList.back();
        m_FreeList.pop_back();
    } else {
        index = static_cast<uint32_t>(m_Generations.size());
        m_Generations.push_back(1); // generation starts at 1
        m_Alive.push_back(0);
    }
    m_Alive[index] = 1;
    return Entity::FromIndexAndGeneration(index, m_Generations[index]);
}

void EntityManager::DestroyEntity(Entity entity) {
    if (!IsEntityValid(entity)) {
        STONEPOTATOES_LOG_WARNING("Attempted to destroy invalid entity");
        return;
    }
    const uint32_t index = entity.GetIndex();
    ++m_Generations[index]; // invalidate outstanding handles
    m_Alive[index] = 0;
    m_FreeList.push_back(index);
}

bool EntityManager::IsEntityValid(Entity entity) const {
    const uint32_t index = entity.GetIndex();
    const uint32_t generation = entity.GetGeneration();
    return index < m_Generations.size() && generation != 0 &&
           m_Generations[index] == generation && m_Alive[index];
}

Entity EntityManager::RestoreEntity(uint32_t index, uint32_t generation) {
    if (index == std::numeric_limits<uint32_t>::max()) {
        STONEPOTATOES_LOG_ERROR("Cannot restore entity with maximum index");
        return Entity{};
    }
    if (generation == 0) {
        generation = 1;
    }
    if (index >= m_Generations.size()) {
        const uint32_t oldSize = static_cast<uint32_t>(m_Generations.size());
        m_Generations.resize(index + 1, 1);
        m_Alive.resize(index + 1, 0);
        for (uint32_t missing = oldSize; missing < index; ++missing) {
            m_FreeList.push_back(missing);
        }
    }
    m_Generations[index] = generation;
    m_Alive[index] = 1;
    // Remove the index from the free list if present.
    for (auto it = m_FreeList.begin(); it != m_FreeList.end(); ++it) {
        if (*it == index) {
            m_FreeList.erase(it);
            break;
        }
    }
    return Entity::FromIndexAndGeneration(index, generation);
}

void EntityManager::ForEachAlive(
    const std::function<void(Entity)>& fn) const {
    for (uint32_t i = 0; i < m_Generations.size(); ++i) {
        if (m_Alive[i]) {
            fn(Entity::FromIndexAndGeneration(i, m_Generations[i]));
        }
    }
}

uint32_t EntityManager::GetEntityCount() const {
    return static_cast<uint32_t>(m_Generations.size() - m_FreeList.size());
}

uint32_t EntityManager::GetCapacity() const {
    return static_cast<uint32_t>(m_Generations.size());
}

} // namespace ECS
} // namespace StonePotatoes
