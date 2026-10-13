#pragma once

#include "Entity.h"
#include <vector>
#include <cstdint>

namespace Genesis {
namespace ECS {

class EntityManager {
public:
    EntityManager();
    ~EntityManager();

    Entity CreateEntity();
    void DestroyEntity(Entity entity);
    bool IsEntityValid(Entity entity) const;

    uint32_t GetEntityCount() const;
    uint32_t GetCapacity() const;

private:
    std::vector<uint32_t> m_Generations;
    std::vector<uint32_t> m_FreeList;
    uint32_t m_NextFreeIndex;
};

} // namespace ECS
} // namespace Genesis
