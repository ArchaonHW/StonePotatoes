#include "ArchetypeManager.h"
#include "../../core/logging/Logger.h"

#include <algorithm>

namespace StonePotatoes {
namespace ECS {

ArchetypeManager::ArchetypeManager() {
    m_EmptyArchetype = GetOrCreateArchetype({});
}

ArchetypeManager::~ArchetypeManager() = default;

Archetype* ArchetypeManager::GetOrCreateArchetype(
    const std::vector<ComponentID>& ids) {
    std::vector<ComponentID> sorted = ids;
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    auto it = m_Archetypes.find(sorted);
    if (it != m_Archetypes.end()) {
        return it->second.get();
    }
    auto arch = std::make_unique<Archetype>(sorted);
    Archetype* ptr = arch.get();
    m_Archetypes.emplace(std::move(sorted), std::move(arch));
    return ptr;
}

Archetype* ArchetypeManager::FindArchetype(
    const std::vector<ComponentID>& ids) const {
    std::vector<ComponentID> sorted = ids;
    std::sort(sorted.begin(), sorted.end());
    auto it = m_Archetypes.find(sorted);
    return it != m_Archetypes.end() ? it->second.get() : nullptr;
}

ArchetypeManager::EntityRecord&
ArchetypeManager::EnsureRecord(Entity entity) {
    const uint32_t index = entity.GetIndex();
    if (index >= m_Records.size()) {
        m_Records.resize(static_cast<size_t>(index) + 1);
    }
    EntityRecord& rec = m_Records[index];
    if (!rec.alive) {
        rec.alive = true;
        rec.archetype = m_EmptyArchetype;
        rec.row = m_EmptyArchetype->AddEntity(entity);
    }
    return rec;
}

void ArchetypeManager::OnEntityDestroyed(Entity entity) {
    const uint32_t index = entity.GetIndex();
    if (index >= m_Records.size() || !m_Records[index].alive) {
        return;
    }
    EntityRecord& rec = m_Records[index];
    Entity moved = rec.archetype->RemoveEntity(rec.row);
    if (moved.IsValid()) {
        m_Records[moved.GetIndex()].row = rec.row;
    }
    rec.alive = false;
    rec.archetype = nullptr;
    rec.row = 0;
}

bool ArchetypeManager::HasRecord(Entity entity) const {
    const uint32_t index = entity.GetIndex();
    return index < m_Records.size() && m_Records[index].alive;
}

const ArchetypeManager::EntityRecord*
ArchetypeManager::GetRecord(Entity entity) const {
    const uint32_t index = entity.GetIndex();
    if (index >= m_Records.size() || !m_Records[index].alive) {
        return nullptr;
    }
    return &m_Records[index];
}

void ArchetypeManager::AddComponentData(Entity entity,
                                      const ComponentTypeInfo* info,
                                      const void* data) {
    if (!info) {
        return;
    }
    EntityRecord& rec = EnsureRecord(entity);
    if (rec.archetype->HasComponent(info->id)) {
        // Overwrite existing component in place (copy, do not consume src).
        void* dst = rec.archetype->GetComponentData(rec.row, info->id);
        info->destroy(dst);
        info->copyConstruct(dst, data);
        return;
    }

    std::vector<ComponentID> newSig = rec.archetype->GetComponentIDs();
    newSig.push_back(info->id);
    MoveEntity(entity, newSig, data, info->id);
}

void ArchetypeManager::RemoveComponentData(Entity entity, ComponentID id) {
    EntityRecord* rec = nullptr;
    {
        const EntityRecord* crec = GetRecord(entity);
        if (!crec || !crec->archetype->HasComponent(id)) {
            return;
        }
        rec = &m_Records[entity.GetIndex()];
    }

    std::vector<ComponentID> newSig = rec->archetype->GetComponentIDs();
    newSig.erase(std::remove(newSig.begin(), newSig.end(), id), newSig.end());
    MoveEntity(entity, newSig, nullptr, 0);
}

void* ArchetypeManager::GetComponentData(Entity entity, ComponentID id) {
    const EntityRecord* rec = GetRecord(entity);
    if (!rec) {
        return nullptr;
    }
    return rec->archetype->GetComponentData(rec->row, id);
}

void ArchetypeManager::MoveEntity(Entity entity,
                                  const std::vector<ComponentID>& newSig,
                                  const void* newData, ComponentID newDataID) {
    EntityRecord& rec = m_Records[entity.GetIndex()];
    Archetype* oldArch = rec.archetype;
    const size_t oldRow = rec.row;

    Archetype* newArch = GetOrCreateArchetype(newSig);
    const size_t newRow = newArch->AddEntity(entity);

    // Copy every column present in the new signature: from the old archetype
    // when it existed there, or from `newData` for the added component.
    for (ComponentID id : newArch->GetComponentIDs()) {
        if (id == newDataID && newData) {
            const ComponentTypeInfo* info = GetComponentInfo(id);
            info->copyConstruct(
                static_cast<uint8_t*>(
                    newArch->GetComponentData(newRow, id)),
                newData);
            // GetComponentData on a fresh row indexes raw storage; safe
            // because EnsureCapacity allocated it in AddEntity.
        } else {
            void* src = oldArch->GetComponentData(oldRow, id);
            if (src) {
                newArch->MoveComponentInto(newRow, id, src);
            }
        }
    }

    // Remove from old archetype; fix the moved entity's row if any.
    Entity moved = oldArch->RemoveEntity(oldRow);
    if (moved.IsValid()) {
        m_Records[moved.GetIndex()].row = oldRow;
    }

    rec.archetype = newArch;
    rec.row = newRow;
}

size_t ArchetypeManager::GetStoredEntityCount() const {
    size_t n = 0;
    for (const auto& rec : m_Records) {
        if (rec.alive) ++n;
    }
    return n;
}

} // namespace ECS
} // namespace StonePotatoes
