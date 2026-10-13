#pragma once

#include "entity/EntityManager.h"
#include "component/ComponentManager.h"
#include "archetype/ArchetypeManager.h"
#include "system/SystemScheduler.h"
#include "events/EventBus.h"
#include "resources/ResourceManager.h"
#include "query/Query.h"

#include <memory>

namespace StonePotatoes {
namespace ECS {

// World facade: entity lifecycle, component storage, systems, events,
// resources, and serialization.
// World 外觀：實體生命週期、組件儲存、系統、事件、資源與序列化。
class World {
public:
    World();
    ~World();

    void Initialize();
    void Update(float deltaTime);
    void Shutdown();

    // --- Entities | 實體 ---
    Entity CreateEntity();
    void DestroyEntity(Entity entity);
    bool IsEntityValid(Entity entity) const;

    // --- Components | 組件 ---
    // Register before first use with a stable serialized name.
    // 首次使用前以穩定的序列化名稱註冊。
    template <typename T>
    const ComponentTypeInfo* RegisterComponent(const char* name) {
        return m_ComponentManager->Register<T>(name);
    }

    template <typename T>
    void AddComponent(Entity entity, const T& component) {
        if (IsEntityValid(entity)) {
            m_ArchetypeManager->AddComponent<T>(entity, component);
        }
    }

    template <typename T>
    void RemoveComponent(Entity entity) {
        if (IsEntityValid(entity)) {
            m_ArchetypeManager->RemoveComponent<T>(entity);
        }
    }

    template <typename T>
    T* GetComponent(Entity entity) {
        if (!IsEntityValid(entity)) {
            return nullptr;
        }
        return m_ArchetypeManager->GetComponent<T>(entity);
    }

    template <typename T>
    bool HasComponent(Entity entity) const {
        return IsEntityValid(entity) &&
               m_ArchetypeManager->HasComponent<T>(entity);
    }

    // fn(Entity, Cs&...) over entities owning all Cs.
    // 對擁有全部 Cs 的實體執行 fn(Entity, Cs&...)。
    template <typename... Cs, typename Fn>
    void ForEach(Fn&& fn) {
        m_ArchetypeManager->ForEach<Cs...>(std::forward<Fn>(fn));
    }

    template <typename... Cs>
    Query<Cs...> MakeQuery() {
        return Query<Cs...>{};
    }

    // --- Events | 事件 ---
    EventBus& GetEventBus() { return *m_EventBus; }

    // --- Resources | 資源 ---
    ResourceManager& GetResourceManager() { return *m_ResourceManager; }

    // --- Persistence | 持久化 ---
    // Binary save/load. Only trivially copyable, named-registered components
    // are serialized; others are skipped with a warning.
    // 二進位儲存/載入。僅序列化已命名註冊且可平凡複製的組件；
    // 其餘組件會被略過並記錄警告。
    bool SaveToFile(const char* path) const;
    bool LoadFromFile(const char* path);

    // --- Subsystem access | 子系統存取 ---
    EntityManager& GetEntityManager();
    ComponentManager& GetComponentManager();
    ArchetypeManager& GetArchetypeManager();
    SystemScheduler& GetSystemScheduler();

private:
    std::unique_ptr<EntityManager> m_EntityManager;
    std::unique_ptr<ComponentManager> m_ComponentManager;
    std::unique_ptr<ArchetypeManager> m_ArchetypeManager;
    std::unique_ptr<SystemScheduler> m_SystemScheduler;
    std::unique_ptr<EventBus> m_EventBus;
    std::unique_ptr<ResourceManager> m_ResourceManager;
    bool m_Shutdown = false;
};

} // namespace ECS
} // namespace StonePotatoes
