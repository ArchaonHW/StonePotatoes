#pragma once

#include "Entity.h"
#include <vector>
#include <cstdint>
#include <functional>

namespace StonePotatoes {
namespace ECS {

// Allocates entity slots and validates handles via generation counters.
// Generations start at 1 so index 0 + gen 1 never collides with the
// id == 0 invalid sentinel.
// 配置實體槽位並以世代計數驗證控制代碼。
// 世代從 1 開始，因此 index 0 + gen 1 不會與 id == 0 的無效哨兵值衝突。
class EntityManager {
public:
    EntityManager();
    ~EntityManager();

    Entity CreateEntity();
    void DestroyEntity(Entity entity);
    bool IsEntityValid(Entity entity) const;

    // Recreate an entity with a known index+generation (used by load).
    // 以指定的 index+generation 重建實體（載入時使用）。
    Entity RestoreEntity(uint32_t index, uint32_t generation);

    // Iterate live entities.
    // 走訪存活的實體。
    void ForEachAlive(const std::function<void(Entity)>& fn) const;

    uint32_t GetEntityCount() const;
    uint32_t GetCapacity() const;

private:
    std::vector<uint32_t> m_Generations;
    std::vector<char>     m_Alive;
    std::vector<uint32_t> m_FreeList; // stack of reusable indices
};

} // namespace ECS
} // namespace StonePotatoes
