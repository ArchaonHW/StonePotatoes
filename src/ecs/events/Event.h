#pragma once

#include <typeindex>

namespace StonePotatoes {
namespace ECS {

// Base class for events. Concrete events are plain structs; inheritance is
// optional — EventBus dispatches by std::type_index, so any type works.
// 事件基底類別。具體事件為純結構體；繼承為非必要 —
// EventBus 以 std::type_index 分派，任何型別皆可用。
struct Event {
    virtual ~Event() = default;
};

// Type key for an event type.
// 事件型別的型別鍵。
template <typename E>
std::type_index EventType() {
    return std::type_index(typeid(E));
}

} // namespace ECS
} // namespace StonePotatoes
