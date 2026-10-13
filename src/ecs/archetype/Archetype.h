#pragma once

#include "../component/Component.h"
#include "../entity/Entity.h"

#include <vector>
#include <unordered_map>

namespace StonePotatoes {
namespace ECS {

// Archetype: all entities sharing the same component set live here.
// Storage is columnar (SoA): each component type owns a packed byte array
// where row i of every column belongs to entities[i].
// Archetype：共享相同組件集合的所有實體存放於此。
// 採用欄式（SoA）儲存：每種組件型別擁有一個緊密位元組陣列，
// 所有欄的第 i 列都屬於 entities[i]。
class Archetype {
public:
    explicit Archetype(std::vector<ComponentID> componentIDs);
    ~Archetype();

    Archetype(const Archetype&) = delete;
    Archetype& operator=(const Archetype&) = delete;

    const std::vector<ComponentID>& GetComponentIDs() const {
        return m_ComponentIDs;
    }
    bool HasComponent(ComponentID id) const;

    // Append an entity row; components start uninitialized — callers must
    // placement-construct each component via WriteComponent or MoveComponent.
    // 追加一個實體列；組件尚未初始化 — 呼叫端必須透過
    // WriteComponent 或 MoveComponent 就地建構各組件。
    size_t AddEntity(Entity entity);

    // Swap-remove a row. If another entity was moved into `row`, returns it
    // so the caller can update its record; otherwise returns INVALID_ENTITY.
    // 交換移除一列。若有另一實體被搬入該列，回傳該實體以便呼叫端更新記錄；
    // 否則回傳 INVALID_ENTITY。
    Entity RemoveEntity(size_t row);

    // Raw component access for a row.
    // 某一列的原始組件存取。
    void* GetComponentData(size_t row, ComponentID id);
    const void* GetComponentData(size_t row, ComponentID id) const;

    // Move-construct a component from an external pointer (used when an
    // entity migrates between archetypes).
    // 從外部指標搬移建構組件（實體在 archetype 之間遷移時使用）。
    void MoveComponentInto(size_t row, ComponentID id, void* src);

    size_t GetEntityCount() const { return m_Entities.size(); }
    Entity GetEntity(size_t row) const { return m_Entities[row]; }

private:
    struct Column {
        const ComponentTypeInfo* info = nullptr;
        std::vector<uint8_t> data; // capacity = m_Capacity * info->size
    };

    void EnsureCapacity(size_t minRows);
    Column* FindColumn(ComponentID id);

    std::vector<ComponentID> m_ComponentIDs; // sorted
    std::unordered_map<ComponentID, size_t> m_ColumnIndex;
    std::vector<Column> m_Columns;
    std::vector<Entity> m_Entities;
    size_t m_Capacity = 0;
};

} // namespace ECS
} // namespace StonePotatoes
