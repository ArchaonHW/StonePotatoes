#pragma once

#include "Component.h"

#include <string>
#include <vector>

namespace StonePotatoes {
namespace ECS {

// World-level component registry facade: tracks which component types this
// world uses and resolves names back to type info for serialization.
// 世界層級的組件註冊介面：追蹤本世界使用的組件型別，
// 並提供名稱到型別資訊的解析以支援序列化。
class ComponentManager {
public:
    ComponentManager();
    ~ComponentManager();

    // Register a component type under a stable serialization name.
    // 以穩定的序列化名稱註冊組件型別。
    template <typename T>
    const ComponentTypeInfo* Register(const char* name) {
        const ComponentTypeInfo* info = RegisterComponent<T>(name);
        m_UsedTypes.push_back(info);
        return info;
    }

    // Resolve a serialized component name back to type info.
    // 將序列化的組件名稱解析回型別資訊。
    const ComponentTypeInfo* FindByName(const std::string& name) const;

    // Types registered through this manager.
    // 透過此管理器註冊的型別。
    const std::vector<const ComponentTypeInfo*>& GetRegisteredTypes() const {
        return m_UsedTypes;
    }

private:
    std::vector<const ComponentTypeInfo*> m_UsedTypes;
};

} // namespace ECS
} // namespace StonePotatoes
