#include "CommandBuffer.h"
#include "../World.h"

namespace StonePotatoes {
namespace ECS {

void CommandBuffer::DestroyEntity(Entity e) {
    Add([e](World& w) { w.DestroyEntity(e); });
}

void CommandBuffer::Apply(World& world) {
    std::vector<std::function<void(World&)>> cmds;
    {
        std::lock_guard<std::mutex> lk(m_Mutex);
        cmds.swap(m_Commands);
    }
    for (auto& fn : cmds) fn(world);
}

} // namespace ECS
} // namespace StonePotatoes
