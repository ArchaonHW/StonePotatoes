#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <new>
#include <string>

namespace StonePotatoes {
namespace ECS {

using ComponentID = uint32_t;

// Runtime metadata for a component type: size/alignment for storage,
// constructors for type-erased moves, and flags for serialization.
// 組件型別的執行期元資料：儲存所需的大小/對齊、
// 型別抹除搬移的建構器，以及序列化旗標。
struct ComponentTypeInfo {
    ComponentID id = 0;
    const char* name = "";
    size_t size = 0;
    size_t alignment = 0;
    bool triviallyCopyable = false;

    // Placement move-construct: new (dst) T(std::move(*src))
    void (*moveConstruct)(void* dst, void* src) = nullptr;
    // Placement copy-construct for serialization of POD types
    void (*copyConstruct)(void* dst, const void* src) = nullptr;
    // Destructor: ptr->~T()
    void (*destroy)(void* ptr) = nullptr;
};

namespace detail {

inline ComponentID& ComponentIDCounter() {
    static ComponentID counter = 1;
    return counter;
}

inline std::unordered_map<ComponentID, const ComponentTypeInfo*>&
ComponentRegistry() {
    static std::unordered_map<ComponentID, const ComponentTypeInfo*> registry;
    return registry;
}

inline std::unordered_map<std::string, const ComponentTypeInfo*>&
ComponentNameRegistry() {
    static std::unordered_map<std::string, const ComponentTypeInfo*> registry;
    return registry;
}

} // namespace detail

// Unique ID per component type (process-local, monotonic).
// 每種組件型別的唯一 ID（行程內遞增）。
template <typename T>
ComponentID GetComponentID() {
    static const ComponentID id = detail::ComponentIDCounter()++;
    return id;
}

// Register component type metadata. Called once per type (idempotent).
// 註冊組件型別元資料。每種型別只需呼叫一次（冪等）。
template <typename T>
const ComponentTypeInfo* RegisterComponent(const char* name) {
    static const ComponentTypeInfo info = [name] {
        ComponentTypeInfo i;
        i.id = GetComponentID<T>();
        i.name = name;
        i.size = sizeof(T);
        i.alignment = alignof(T);
        i.triviallyCopyable = std::is_trivially_copyable_v<T>;
        i.moveConstruct = [](void* dst, void* src) {
            ::new (dst) T(std::move(*static_cast<T*>(src)));
        };
        i.copyConstruct = [](void* dst, const void* src) {
            ::new (dst) T(*static_cast<const T*>(src));
        };
        i.destroy = [](void* ptr) { static_cast<T*>(ptr)->~T(); };
        return i;
    }();
    detail::ComponentRegistry()[info.id] = &info;
    detail::ComponentNameRegistry()[info.name] = &info;
    return &info;
}

// Lookup registered info by ID or by serialized name.
// 以 ID 或序列化名稱查詢註冊資訊。
inline const ComponentTypeInfo* GetComponentInfo(ComponentID id) {
    auto it = detail::ComponentRegistry().find(id);
    return it != detail::ComponentRegistry().end() ? it->second : nullptr;
}

inline const ComponentTypeInfo* GetComponentInfoByName(const std::string& name) {
    auto it = detail::ComponentNameRegistry().find(name);
    return it != detail::ComponentNameRegistry().end() ? it->second : nullptr;
}

// Convenience: get (registering on first use) the type info for T.
// 便捷函式：取得 T 的型別資訊（首次使用時自動註冊）。
template <typename T>
const ComponentTypeInfo* ComponentInfo() {
    static const ComponentTypeInfo* info =
        RegisterComponent<T>(typeid(T).name());
    return info;
}

} // namespace ECS
} // namespace StonePotatoes
