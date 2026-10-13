#include "Archetype.h"
#include "../../core/logging/Logger.h"

#include <algorithm>
#include <cstring>

namespace StonePotatoes {
namespace ECS {

Archetype::Archetype(std::vector<ComponentID> componentIDs)
    : m_ComponentIDs(std::move(componentIDs)) {
    std::sort(m_ComponentIDs.begin(), m_ComponentIDs.end());
    m_Columns.reserve(m_ComponentIDs.size());
    for (ComponentID id : m_ComponentIDs) {
        const ComponentTypeInfo* info = GetComponentInfo(id);
        if (!info) {
            STONEPOTATOES_LOG_ERROR(
                "Archetype: component %u not registered", id);
            continue;
        }
        m_ColumnIndex[id] = m_Columns.size();
        Column col;
        col.info = info;
        m_Columns.push_back(std::move(col));
    }
    m_Capacity = 0;
}

Archetype::~Archetype() {
    // Destroy live component objects
    for (size_t row = 0; row < m_Entities.size(); ++row) {
        for (auto& col : m_Columns) {
            col.info->destroy(col.data.data() + row * col.info->size);
        }
    }
}

bool Archetype::HasComponent(ComponentID id) const {
    return m_ColumnIndex.find(id) != m_ColumnIndex.end();
}

void Archetype::EnsureCapacity(size_t minRows) {
    if (minRows <= m_Capacity) {
        return;
    }
    const size_t newCap = m_Capacity == 0 ? 8 : m_Capacity * 2;
    const size_t cap = newCap < minRows ? minRows : newCap;
    for (auto& col : m_Columns) {
        std::vector<uint8_t> grown(cap * col.info->size);
        // Move existing rows
        for (size_t row = 0; row < m_Entities.size(); ++row) {
            void* dst = grown.data() + row * col.info->size;
            void* src = col.data.data() + row * col.info->size;
            col.info->moveConstruct(dst, src);
        }
        // Destroy old
        for (size_t row = 0; row < m_Entities.size(); ++row) {
            col.info->destroy(col.data.data() + row * col.info->size);
        }
        col.data = std::move(grown);
    }
    m_Capacity = cap;
}

size_t Archetype::AddEntity(Entity entity) {
    EnsureCapacity(m_Entities.size() + 1);
    m_Entities.push_back(entity);
    return m_Entities.size() - 1;
}

Entity Archetype::RemoveEntity(size_t row) {
    if (row >= m_Entities.size()) {
        return INVALID_ENTITY;
    }
    const size_t last = m_Entities.size() - 1;
    // Destroy this row's components
    for (auto& col : m_Columns) {
        col.info->destroy(col.data.data() + row * col.info->size);
    }
    // Swap last row into `row`
    if (row != last) {
        for (auto& col : m_Columns) {
            void* dst = col.data.data() + row * col.info->size;
            void* src = col.data.data() + last * col.info->size;
            col.info->moveConstruct(dst, src);
            col.info->destroy(src);
        }
        const Entity moved = m_Entities[last];
        m_Entities[row] = moved;
        m_Entities.pop_back();
        return moved;
    }
    m_Entities.pop_back();
    return INVALID_ENTITY;
}

void* Archetype::GetComponentData(size_t row, ComponentID id) {
    Column* col = FindColumn(id);
    if (!col || row >= m_Entities.size()) {
        return nullptr;
    }
    return col->data.data() + row * col->info->size;
}

const void* Archetype::GetComponentData(size_t row, ComponentID id) const {
    auto it = m_ColumnIndex.find(id);
    if (it == m_ColumnIndex.end() || row >= m_Entities.size()) {
        return nullptr;
    }
    const Column& col = m_Columns[it->second];
    return col.data.data() + row * col.info->size;
}

void Archetype::MoveComponentInto(size_t row, ComponentID id, void* src) {
    Column* col = FindColumn(id);
    if (!col || row >= m_Capacity) {
        return;
    }
    col->info->moveConstruct(col->data.data() + row * col->info->size, src);
}

Archetype::Column* Archetype::FindColumn(ComponentID id) {
    auto it = m_ColumnIndex.find(id);
    return it != m_ColumnIndex.end() ? &m_Columns[it->second] : nullptr;
}

} // namespace ECS
} // namespace StonePotatoes
