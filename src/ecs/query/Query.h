#pragma once

#include "../archetype/ArchetypeManager.h"
#include "../component/Component.h"

#include <vector>

namespace StonePotatoes {
namespace ECS {

// Query over archetypes: requires all `Required` components and supports
// optional With/Without filters.
// 針對 archetype 的查詢：必須擁有全部 `Required` 組件，
// 並支援選擇性的 With/Without 篩選。
template <typename... Required>
class Query {
public:
    // Additional required/excluded sets passed as ComponentID lists so a
    // query can be refined at runtime (e.g. Query<T>::With<U>()).
    // 以 ComponentID 清單傳入額外的必要/排除集合，
    // 讓查詢可在執行期調整（例如 Query<T>::With<U>()）。
    template <typename... With>
    Query& And() {
        (m_With.push_back(GetComponentID<With>()), ...);
        return *this;
    }

    template <typename... Without>
    Query& Except() {
        (m_Without.push_back(GetComponentID<Without>()), ...);
        return *this;
    }

    // fn(Entity, Required&...)
    template <typename Fn>
    void Each(ArchetypeManager& mgr, Fn&& fn) {
        mgr.ForEachFiltered<Required...>(
            m_With, m_Without, std::forward<Fn>(fn));
    }

    // Collect matching entities.
    // 收集符合條件的實體。
    std::vector<Entity> Collect(ArchetypeManager& mgr) {
        std::vector<Entity> out;
        Each(mgr, [&out](Entity e, Required&...) { out.push_back(e); });
        return out;
    }

    size_t Count(ArchetypeManager& mgr) {
        size_t n = 0;
        Each(mgr, [&n](Entity, Required&...) { ++n; });
        return n;
    }

private:
    std::vector<ComponentID> m_With;
    std::vector<ComponentID> m_Without;
};

} // namespace ECS
} // namespace StonePotatoes
