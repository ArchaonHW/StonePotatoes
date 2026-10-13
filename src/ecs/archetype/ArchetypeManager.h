#pragma once

#include "Archetype.h"
#include "../component/Component.h"
#include "../entity/Entity.h"

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace StonePotatoes {
namespace ECS {

// Tracks which archetype/row each entity lives in, and moves entities
// between archetypes as components are added or removed.
// 追蹤每個實體所在的 archetype 與列，
// 並在組件新增/移除時於 archetype 之間搬移實體。
class ArchetypeManager {
public:
    ArchetypeManager();
    ~ArchetypeManager();

    struct EntityRecord {
        Archetype* archetype = nullptr;
        size_t row = 0;
        bool alive = false;
    };

    Archetype* GetOrCreateArchetype(const std::vector<ComponentID>& ids);
    Archetype* FindArchetype(const std::vector<ComponentID>& ids) const;

    // Entity lifecycle within storage (independent of generation checks,
    // which EntityManager owns).
    // 儲存層的實體生命週期（世代檢查由 EntityManager 負責）。
    EntityRecord& EnsureRecord(Entity entity);
    void OnEntityDestroyed(Entity entity);
    bool HasRecord(Entity entity) const;
    const EntityRecord* GetRecord(Entity entity) const;

    // Typed component ops. T must be registered (ComponentInfo<T>()).
    // 有型別的組件操作。T 必須已註冊（ComponentInfo<T>()）。
    template <typename T>
    void AddComponent(Entity entity, const T& component) {
        const ComponentTypeInfo* info = ComponentInfo<T>();
        AddComponentData(entity, info, &component);
    }

    template <typename T>
    void RemoveComponent(Entity entity) {
        RemoveComponentData(entity, GetComponentID<T>());
    }

    template <typename T>
    T* GetComponent(Entity entity) {
        return static_cast<T*>(GetComponentData(entity, GetComponentID<T>()));
    }

    template <typename T>
    const T* GetComponent(Entity entity) const {
        return static_cast<const T*>(
            GetComponentData(entity, GetComponentID<T>()));
    }

    template <typename T>
    bool HasComponent(Entity entity) const {
        const EntityRecord* rec = GetRecord(entity);
        return rec && rec->archetype &&
               rec->archetype->HasComponent(GetComponentID<T>());
    }

    // Type-erased variants used by migration and serialization.
    // 供遷移與序列化使用的型別抹除版本。
    void AddComponentData(Entity entity, const ComponentTypeInfo* info,
                          const void* data);
    void RemoveComponentData(Entity entity, ComponentID id);
    void* GetComponentData(Entity entity, ComponentID id);

    // Iterate entities having all of Cs...; fn(Entity, Cs&...)
    // 走訪擁有全部 Cs... 的實體；fn(Entity, Cs&...)
    template <typename... Cs, typename Fn>
    void ForEach(Fn&& fn) {
        ForEachImpl<Cs...>(std::forward<Fn>(fn), nullptr, nullptr,
                           std::make_index_sequence<sizeof...(Cs)>{});
    }

    // ForEach with runtime with/without filters (used by Query).
    // 附帶執行期 with/without 篩選的 ForEach（供 Query 使用）。
    template <typename... Cs, typename Fn>
    void ForEachFiltered(const std::vector<ComponentID>& with,
                         const std::vector<ComponentID>& without, Fn&& fn) {
        ForEachImpl<Cs...>(std::forward<Fn>(fn), &with, &without,
                           std::make_index_sequence<sizeof...(Cs)>{});
    }

    // All archetypes (for serialization/iteration).
    // 所有 archetype（供序列化/走訪）。
    const std::map<std::vector<ComponentID>, std::unique_ptr<Archetype>>&
    GetArchetypes() const {
        return m_Archetypes;
    }

    size_t GetArchetypeCount() const { return m_Archetypes.size(); }
    size_t GetStoredEntityCount() const;

private:
    void MoveEntity(Entity entity, const std::vector<ComponentID>& newSig,
                    const void* newData, ComponentID newDataID);

    template <typename... Cs, typename Fn, size_t... Is>
    void ForEachImpl(Fn&& fn, const std::vector<ComponentID>* with,
                     const std::vector<ComponentID>* without,
                     std::index_sequence<Is...>) {
        const ComponentID required[] = {GetComponentID<Cs>()...};
        for (auto& kv : m_Archetypes) {
            Archetype* arch = kv.second.get();
            bool match = true;
            for (ComponentID id : required) {
                if (!arch->HasComponent(id)) {
                    match = false;
                    break;
                }
            }
            if (match && with) {
                for (ComponentID id : *with) {
                    if (!arch->HasComponent(id)) {
                        match = false;
                        break;
                    }
                }
            }
            if (match && without) {
                for (ComponentID id : *without) {
                    if (arch->HasComponent(id)) {
                        match = false;
                        break;
                    }
                }
            }
            if (!match) {
                continue;
            }
            const size_t rows = arch->GetEntityCount();
            for (size_t row = 0; row < rows; ++row) {
                fn(arch->GetEntity(row),
                   *static_cast<Cs*>(arch->GetComponentData(
                       row, GetComponentID<Cs>()))...);
            }
        }
    }

    std::map<std::vector<ComponentID>, std::unique_ptr<Archetype>> m_Archetypes;
    // Records indexed by entity index (vector grows as needed).
    // 以實體索引為下標的記錄表（視需要成長）。
    std::vector<EntityRecord> m_Records;
    Archetype* m_EmptyArchetype = nullptr; // archetype with no components
};

} // namespace ECS
} // namespace StonePotatoes
