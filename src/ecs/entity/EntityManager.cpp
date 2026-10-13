#include "EntityManager.h"
#include "../../core/logging/Logger.h"

namespace StonePotatoes {
namespace ECS {

EntityManager::EntityManager()
    : m_NextFreeIndex(0) {
    // Reserve space for entities
    m_Generations.reserve(1024);
}

EntityManager::~EntityManager() {
}

Entity EntityManager::CreateEntity() {
    uint32_t index;

    if (m_NextFreeIndex < m_FreeList.size()) {
        // Reuse a free slot
        index = m_FreeList[m_NextFreeIndex++];
    } else {
        // Allocate new slot
        index = static_cast<uint32_t>(m_Generations.size());
        m_Generations.push_back(0);
    }

    uint32_t generation = m_Generations[index]++;
    return Entity::FromIndexAndGeneration(index, generation);
}

void EntityManager::DestroyEntity(Entity entity) {
    if (!IsEntityValid(entity)) {
        STONEPOTATOES_LOG_WARNING("Attempted to destroy invalid entity");
        return;
    }

    uint32_t index = entity.GetIndex();
    m_Generations[index]++; // Increment generation to invalidate old references

    // Add to free list
    if (m_NextFreeIndex < m_FreeList.size()) {
        m_FreeList[m_NextFreeIndex++] = index;
    } else {
        m_FreeList.push_back(index);
        m_NextFreeIndex++;
    }
}

bool EntityManager::IsEntityValid(Entity entity) const {
    uint32_t index = entity.GetIndex();
    uint32_t generation = entity.GetGeneration();

    if (index >= m_Generations.size()) {
        return false;
    }

    return m_Generations[index] == generation;
}

uint32_t EntityManager::GetEntityCount() const {
    return static_cast<uint32_t>(m_Generations.size() - m_NextFreeIndex);
}

uint32_t EntityManager::GetCapacity() const {
    return static_cast<uint32_t>(m_Generations.size());
}

} // namespace ECS
} // namespace StonePotatoes
