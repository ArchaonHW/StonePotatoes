#pragma once

#include "../entity/Entity.h"

#include <functional>
#include <mutex>
#include <vector>

namespace StonePotatoes {
namespace ECS {

class World;

// Deferred structural ops for parallel execution. Systems running in
// the same wave must not create/destroy entities or migrate components
// mid-iteration — they queue commands here; the world applies them at
// wave boundaries (and at end of a sequential Update).
// 平行執行用的延遲結構操作。同波並行的系統不得在走訪中建立/銷毀實體
// 或遷移組件 — 它們把指令排入此佇列；世界於波次邊界
// （以及循序 Update 結尾）統一執行。
class CommandBuffer {
public:
    CommandBuffer() = default;

    // Thread-safe enqueue (multiple wave systems may queue at once).
    // 執行緒安全入列（同波多系統可能同時排入）。
    void Add(std::function<void(World&)> fn) {
        std::lock_guard<std::mutex> lk(m_Mutex);
        m_Commands.push_back(std::move(fn));
    }

    void DestroyEntity(Entity e);   // out-of-line (World incomplete)

    size_t Pending() const {
        std::lock_guard<std::mutex> lk(m_Mutex);
        return m_Commands.size();
    }

    void Apply(World& world);

    void Clear() {
        std::lock_guard<std::mutex> lk(m_Mutex);
        m_Commands.clear();
    }

private:
    mutable std::mutex m_Mutex;
    std::vector<std::function<void(World&)>> m_Commands;
};

} // namespace ECS
} // namespace StonePotatoes
