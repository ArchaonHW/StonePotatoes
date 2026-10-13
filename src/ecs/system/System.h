#pragma once

#include <typeindex>
#include <vector>

namespace StonePotatoes {
namespace ECS {

class World;

// Declared component access of a system — used by the parallel
// scheduler to decide which systems may run concurrently.
// 系統宣告的組件存取 — 供平行排程器判斷哪些系統可並行執行。
struct AccessSpec {
    struct Entry {
        std::type_index type;
        bool write;
    };
    std::vector<Entry> entries;

    // Performs structural changes (create/destroy entities, add/remove
    // components) → must run in its own wave.
    // 會做結構性變更（建立/銷毀實體、增刪組件）→ 必須獨佔一個 wave。
    bool structural = false;

    template <typename T> void Read()  { entries.push_back({typeid(T), false}); }
    template <typename T> void Write() { entries.push_back({typeid(T), true}); }
};

class System {
public:
    virtual ~System() = default;

    virtual void Initialize(World* world) { (void)world; }
    virtual void Update(World* world, float deltaTime) = 0;
    virtual void Shutdown(World* world) { (void)world; }

    // Parallel scheduling contract: declare every component type the
    // system reads or writes. A system that declares nothing is
    // treated as a serial barrier (conservative default).
    // 平行排程契約：宣告系統讀寫的所有組件型別。
    // 未宣告任何存取的系統視為序列屏障（保守預設）。
    virtual void DescribeAccess(AccessSpec&) const {}
};

} // namespace ECS
} // namespace StonePotatoes
