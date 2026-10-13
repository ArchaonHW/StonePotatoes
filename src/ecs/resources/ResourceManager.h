#pragma once

#include "Resource.h"

#include <memory>
#include <typeindex>
#include <unordered_map>

namespace StonePotatoes {
namespace ECS {

// Type-indexed singleton storage for world resources.
// 以型別索引的世界資源單例儲存。
class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    template <typename T, typename... Args>
    T& Insert(Args&&... args) {
        auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
        T& ref = *ptr;
        m_Resources[std::type_index(typeid(T))] = std::move(ptr);
        return ref;
    }

    template <typename T>
    void InsertPtr(std::shared_ptr<T> ptr) {
        m_Resources[std::type_index(typeid(T))] = std::move(ptr);
    }

    template <typename T>
    T* Get() {
        auto it = m_Resources.find(std::type_index(typeid(T)));
        return it != m_Resources.end()
                   ? static_cast<T*>(it->second.get())
                   : nullptr;
    }

    template <typename T>
    const T* Get() const {
        auto it = m_Resources.find(std::type_index(typeid(T)));
        return it != m_Resources.end()
                   ? static_cast<const T*>(it->second.get())
                   : nullptr;
    }

    template <typename T>
    bool Has() const {
        return m_Resources.find(std::type_index(typeid(T))) !=
               m_Resources.end();
    }

    template <typename T>
    bool Remove() {
        return m_Resources.erase(std::type_index(typeid(T))) > 0;
    }

    void Clear() { m_Resources.clear(); }
    size_t Count() const { return m_Resources.size(); }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_Resources;
};

} // namespace ECS
} // namespace StonePotatoes
