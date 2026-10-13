#include "World.h"
#include "../core/logging/Logger.h"
#include "../core/io/FileSystem.h"

#include <cstring>

namespace StonePotatoes {
namespace ECS {

namespace {

constexpr uint32_t kSaveMagic = 0x53505744;   // 'SPWD'
constexpr uint32_t kSaveVersion = 1;

void WriteU32(IO::File& f, uint32_t v) { f.Write(&v, sizeof(v)); }
void WriteU16(IO::File& f, uint16_t v) { f.Write(&v, sizeof(v)); }

bool ReadU32(IO::File& f, uint32_t& v) {
    return f.Read(&v, sizeof(v)) == sizeof(v);
}
bool ReadU16(IO::File& f, uint16_t& v) {
    return f.Read(&v, sizeof(v)) == sizeof(v);
}

} // namespace

World::World()
    : m_EntityManager(std::make_unique<EntityManager>())
    , m_ComponentManager(std::make_unique<ComponentManager>())
    , m_ArchetypeManager(std::make_unique<ArchetypeManager>())
    , m_SystemScheduler(std::make_unique<SystemScheduler>())
    , m_EventBus(std::make_unique<EventBus>())
    , m_ResourceManager(std::make_unique<ResourceManager>())
    , m_CommandBuffer(std::make_unique<CommandBuffer>()) {
}

World::~World() {
    Shutdown();
}

void World::Initialize() {
    STONEPOTATOES_LOG_INFO("World: Initializing");
    m_SystemScheduler->InitializeSystems(this);
    STONEPOTATOES_LOG_INFO("World: Initialized");
}

void World::Update(float deltaTime) {
    m_SystemScheduler->UpdateSystems(this, deltaTime);
    m_CommandBuffer->Apply(*this);
    m_EventBus->Dispatch(); // deliver queued events at end of tick
}

SystemScheduler::ParallelStats
World::UpdateParallel(float deltaTime, const ParallelForFn& parallelFor) {
    auto stats =
        m_SystemScheduler->UpdateSystemsParallel(this, deltaTime,
                                                 parallelFor);
    m_CommandBuffer->Apply(*this);   // flush tail commands | 收尾指令
    m_EventBus->Dispatch();
    return stats;
}

void World::Shutdown() {
    if (m_Shutdown) return;
    m_Shutdown = true;
    STONEPOTATOES_LOG_INFO("World: Shutting down");
    m_SystemScheduler->ShutdownSystems(this);
    STONEPOTATOES_LOG_INFO("World: Shutdown complete");
}

Entity World::CreateEntity() {
    Entity e = m_EntityManager->CreateEntity();
    m_ArchetypeManager->EnsureRecord(e);
    return e;
}

void World::DestroyEntity(Entity entity) {
    if (!IsEntityValid(entity)) {
        return;
    }
    m_ArchetypeManager->OnEntityDestroyed(entity);
    m_EntityManager->DestroyEntity(entity);
}

bool World::IsEntityValid(Entity entity) const {
    return m_EntityManager->IsEntityValid(entity);
}

// ---------------------------------------------------------------------------
// Serialization
// Format: magic u32 | version u32 | entityCount u32 |
//   per entity: index u32 | generation u32 | componentCount u32 |
//     per component: nameLen u16 | name | dataSize u32 | data bytes
// ---------------------------------------------------------------------------

bool World::SaveToFile(const char* path) const {
    IO::File file;
    if (!file.Open(path, IO::FileMode::Write)) {
        STONEPOTATOES_LOG_ERROR("World: cannot open %s for save", path);
        return false;
    }

    WriteU32(file, kSaveMagic);
    WriteU32(file, kSaveVersion);
    WriteU32(file, m_EntityManager->GetEntityCount());

    m_EntityManager->ForEachAlive([&](Entity e) {
        WriteU32(file, e.GetIndex());
        WriteU32(file, e.GetGeneration());

        const auto* rec = m_ArchetypeManager->GetRecord(e);
        if (!rec || !rec->archetype) {
            WriteU32(file, 0);
            return;
        }

        // Count serializable components first.
        const auto& ids = rec->archetype->GetComponentIDs();
        uint32_t count = 0;
        for (ComponentID id : ids) {
            const ComponentTypeInfo* info = GetComponentInfo(id);
            if (info && info->triviallyCopyable) {
                ++count;
            }
        }
        WriteU32(file, count);

        for (ComponentID id : ids) {
            const ComponentTypeInfo* info = GetComponentInfo(id);
            if (!info || !info->triviallyCopyable) {
                if (info) {
                    STONEPOTATOES_LOG_WARNING(
                        "World::Save skipping non-POD component %s",
                        info->name);
                }
                continue;
            }
            const void* data =
                rec->archetype->GetComponentData(rec->row, id);
            const size_t nameLen = std::strlen(info->name);
            WriteU16(file, static_cast<uint16_t>(nameLen));
            file.Write(info->name, nameLen);
            WriteU32(file, static_cast<uint32_t>(info->size));
            file.Write(data, info->size);
        }
    });

    STONEPOTATOES_LOG_INFO("World: saved %u entities to %s",
                           m_EntityManager->GetEntityCount(), path);
    return true;
}

bool World::LoadFromFile(const char* path) {
    IO::File file;
    if (!file.Open(path, IO::FileMode::Read)) {
        STONEPOTATOES_LOG_ERROR("World: cannot open %s for load", path);
        return false;
    }

    uint32_t magic = 0, version = 0, entityCount = 0;
    if (!ReadU32(file, magic) || magic != kSaveMagic ||
        !ReadU32(file, version) || version != kSaveVersion ||
        !ReadU32(file, entityCount)) {
        STONEPOTATOES_LOG_ERROR("World: %s is not a valid save file", path);
        return false;
    }

    for (uint32_t i = 0; i < entityCount; ++i) {
        uint32_t index = 0, generation = 0, componentCount = 0;
        if (!ReadU32(file, index) || !ReadU32(file, generation) ||
            !ReadU32(file, componentCount)) {
            STONEPOTATOES_LOG_ERROR("World: truncated save file at entity %u",
                                    i);
            return false;
        }
        Entity e = m_EntityManager->RestoreEntity(index, generation);
        m_ArchetypeManager->EnsureRecord(e);

        for (uint32_t c = 0; c < componentCount; ++c) {
            uint16_t nameLen = 0;
            if (!ReadU16(file, nameLen)) {
                return false;
            }
            std::string name(nameLen, '\0');
            if (file.Read(name.data(), nameLen) != nameLen) {
                return false;
            }
            uint32_t dataSize = 0;
            if (!ReadU32(file, dataSize)) {
                return false;
            }
            const ComponentTypeInfo* info =
                m_ComponentManager->FindByName(name);
            if (!info || info->size != dataSize) {
                STONEPOTATOES_LOG_WARNING(
                    "World: unknown/mismatched component '%s', skipping",
                    name.c_str());
                file.Seek(dataSize, IO::SeekOrigin::Current);
                continue;
            }
            // Read into a temp buffer then route through AddComponentData so
            // the entity migrates to the correct archetype.
            std::vector<uint8_t> buf(dataSize);
            if (file.Read(buf.data(), dataSize) != dataSize) {
                return false;
            }
            m_ArchetypeManager->AddComponentData(e, info, buf.data());
        }
    }

    STONEPOTATOES_LOG_INFO("World: loaded %u entities from %s",
                           entityCount, path);
    return true;
}

EntityManager& World::GetEntityManager() {
    return *m_EntityManager;
}

ComponentManager& World::GetComponentManager() {
    return *m_ComponentManager;
}

ArchetypeManager& World::GetArchetypeManager() {
    return *m_ArchetypeManager;
}

SystemScheduler& World::GetSystemScheduler() {
    return *m_SystemScheduler;
}

} // namespace ECS
} // namespace StonePotatoes
