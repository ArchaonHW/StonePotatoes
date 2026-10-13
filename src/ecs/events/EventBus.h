#pragma once

#include "Event.h"

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace StonePotatoes {
namespace ECS {

// Type-indexed event bus with immediate and deferred dispatch.
// Subscribe<E>(fn) registers a handler; Publish<E>(e) fires immediately;
// Queue<E>(e) stores it for Dispatch() at the end of the frame.
// 以型別索引的事件匯流排，支援即時與延遲分派。
// Subscribe<E>(fn) 註冊處理器；Publish<E>(e) 立即觸發；
// Queue<E>(e) 儲存事件，由 Dispatch() 於幀末統一處理。
class EventBus {
public:
    EventBus() = default;
    ~EventBus() = default;

    using HandlerID = size_t;

    template <typename E>
    HandlerID Subscribe(std::function<void(const E&)> handler) {
        auto& list = m_Handlers[EventType<E>()];
        const HandlerID id = m_NextHandlerID++;
        list.push_back({id, [h = std::move(handler)](const void* e) {
            h(*static_cast<const E*>(e));
        }});
        return id;
    }

    bool Unsubscribe(std::type_index type, HandlerID id);

    // Fire event to all subscribers of E now.
    // 立即將事件傳給 E 的所有訂閱者。
    template <typename E>
    void Publish(const E& event) {
        auto it = m_Handlers.find(EventType<E>());
        if (it == m_Handlers.end()) {
            return;
        }
        // Copy handler list: a handler may subscribe/unsubscribe mid-publish.
        auto handlers = it->second;
        for (auto& h : handlers) {
            h.fn(&event);
        }
    }

    // Store event for later dispatch (copy captured, type-erased).
    // 儲存事件待稍後分派（複製並型別抹除）。
    template <typename E>
    void Queue(E event) {
        m_Deferred.push_back([this, e = std::move(event)]() mutable {
            Publish<E>(e);
        });
    }

    // Deliver all queued events.
    // 送出所有佇列中的事件。
    void Dispatch();

    void Clear();

private:
    struct TypeErasedHandler {
        HandlerID id;
        std::function<void(const void*)> fn;
    };

    std::unordered_map<std::type_index, std::vector<TypeErasedHandler>>
        m_Handlers;
    std::vector<std::function<void()>> m_Deferred;
    HandlerID m_NextHandlerID = 1;
};

} // namespace ECS
} // namespace StonePotatoes
