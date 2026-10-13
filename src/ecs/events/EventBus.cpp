#include "EventBus.h"

#include <algorithm>

namespace StonePotatoes {
namespace ECS {

bool EventBus::Unsubscribe(std::type_index type, HandlerID id) {
    auto it = m_Handlers.find(type);
    if (it == m_Handlers.end()) {
        return false;
    }
    auto& list = it->second;
    auto pos = std::find_if(list.begin(), list.end(),
                            [id](const TypeErasedHandler& h) {
                                return h.id == id;
                            });
    if (pos == list.end()) {
        return false;
    }
    list.erase(pos);
    return true;
}

void EventBus::Dispatch() {
    // Swap out the queue so handlers may queue new events safely.
    std::vector<std::function<void()>> pending;
    pending.swap(m_Deferred);
    for (auto& fn : pending) {
        fn();
    }
}

void EventBus::Clear() {
    m_Handlers.clear();
    m_Deferred.clear();
}

} // namespace ECS
} // namespace StonePotatoes
