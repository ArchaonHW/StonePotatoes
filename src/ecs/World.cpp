#include "World.h"
#include "../core/logging/Logger.h"
#include "../core/io/FileSystem.h"

#include <cstring>
#include <limits>
#include <unordered_set>
#include <utility>

namespace StonePotatoes {
namespace ECS {

namespace {

constexpr uint32_t kSaveMagic = 0x53505744;   // 'SPWD'
constexpr uint32_t kSaveVersion = 1;

void AppendU16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8));
}

void AppendU32(std::vector<uint8_t>& out, uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<uint8_t>(value >> shift));
    }
}

bool ReadU16(const std::vector<uint8_t>& bytes, size_t& offset,
             uint16_t& value) {
    if (bytes.size() - offset < 2) return false;
    value = static_cast<uint16_t>(
        static_cast<uint16_t>(bytes[offset]) |
        static_cast<uint16_t>(static_cast<uint16_t>(bytes[offset + 1]) << 8));
    offset += 2;
    return true;
}

bool ReadU32(const std::vector<uint8_t>& bytes, size_t& offset,
             uint32_t& value) {
    if (bytes.size() - offset < 4) return false;
    value = 0;
    for (unsigned shift = 0; shift < 32; shift += 8) {
        value |= static_cast<uint32_t>(bytes[offset++]) << shift;
    }
    return true;
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
    if (!path) {
        STONEPOTATOES_LOG_ERROR("World: cannot save to a null path");
        return false;
    }

    std::vector<uint8_t> bytes;
    AppendU32(bytes, kSaveMagic);
    AppendU32(bytes, kSaveVersion);
    AppendU32(bytes, m_EntityManager->GetEntityCount());

    bool valid = true;
    m_EntityManager->ForEachAlive([&](Entity e) {
        if (!valid) return;
        AppendU32(bytes, e.GetIndex());
        AppendU32(bytes, e.GetGeneration());

        const auto* rec = m_ArchetypeManager->GetRecord(e);
        if (!rec || !rec->archetype) {
            AppendU32(bytes, 0);
            return;
        }

        const auto& ids = rec->archetype->GetComponentIDs();
        uint32_t count = 0;
        for (ComponentID id : ids) {
            const ComponentTypeInfo* info = GetComponentInfo(id);
            if (info && info->triviallyCopyable) ++count;
        }
        AppendU32(bytes, count);

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

            const size_t nameLen = std::strlen(info->name);
            if (nameLen > std::numeric_limits<uint16_t>::max() ||
                info->size > std::numeric_limits<uint32_t>::max()) {
                STONEPOTATOES_LOG_ERROR(
                    "World: component %s exceeds save format limits",
                    info->name);
                valid = false;
                return;
            }
            AppendU16(bytes, static_cast<uint16_t>(nameLen));
            bytes.insert(bytes.end(), info->name, info->name + nameLen);
            AppendU32(bytes, static_cast<uint32_t>(info->size));
            const auto* data = static_cast<const uint8_t*>(
                rec->archetype->GetComponentData(rec->row, id));
            bytes.insert(bytes.end(), data, data + info->size);
        }
    });

    if (!valid) return false;
    if (!IO::FileSystem::WriteBinaryFile(path, bytes.data(), bytes.size())) {
        STONEPOTATOES_LOG_ERROR("World: cannot write save file %s", path);
        return false;
    }

    STONEPOTATOES_LOG_INFO("World: saved %u entities to %s",
                           m_EntityManager->GetEntityCount(), path);
    return true;
}

bool World::LoadFromFile(const char* path) {
    if (!path) {
        STONEPOTATOES_LOG_ERROR("World: cannot load from a null path");
        return false;
    }
    IO::File file;
    if (!file.Open(path, IO::FileMode::Read)) {
        STONEPOTATOES_LOG_ERROR("World: cannot open %s for load", path);
        return false;
    }

    const int64_t fileSize = file.Size();
    if (fileSize < 0 || static_cast<uint64_t>(fileSize) >
                            std::numeric_limits<size_t>::max()) {
        return false;
    }
    std::vector<uint8_t> bytes(static_cast<size_t>(fileSize));
    if (file.Read(bytes.data(), bytes.size()) != bytes.size()) {
        return false;
    }

    size_t offset = 0;
    uint32_t magic = 0, version = 0, entityCount = 0;
    if (!ReadU32(bytes, offset, magic) || magic != kSaveMagic ||
        !ReadU32(bytes, offset, version) || version != kSaveVersion ||
        !ReadU32(bytes, offset, entityCount)) {
        STONEPOTATOES_LOG_ERROR("World: %s is not a valid save file", path);
        return false;
    }

    struct SavedComponent {
        const ComponentTypeInfo* info;
        std::vector<uint8_t> data;
    };
    struct SavedEntity {
        uint32_t index;
        uint32_t generation;
        std::vector<SavedComponent> components;
    };
    // Every entity needs at least its three u32 fields.
    if (entityCount > (bytes.size() - offset) / 12) {
        STONEPOTATOES_LOG_ERROR("World: invalid entity count in %s", path);
        return false;
    }
    std::vector<SavedEntity> entities;
    entities.reserve(entityCount);
    std::unordered_set<uint32_t> entityIndices;

    for (uint32_t i = 0; i < entityCount; ++i) {
        uint32_t index = 0, generation = 0, componentCount = 0;
        if (!ReadU32(bytes, offset, index) ||
            !ReadU32(bytes, offset, generation) ||
            !ReadU32(bytes, offset, componentCount) || generation == 0 ||
            index == std::numeric_limits<uint32_t>::max() ||
            !entityIndices.insert(index).second) {
            STONEPOTATOES_LOG_ERROR("World: truncated save file at entity %u",
                                    i);
            return false;
        }

        if (componentCount > (bytes.size() - offset) / 6) {
            STONEPOTATOES_LOG_ERROR("World: invalid component count at entity %u",
                                    i);
            return false;
        }
        SavedEntity saved{index, generation, {}};
        saved.components.reserve(componentCount);
        std::unordered_set<std::string> componentNames;

        for (uint32_t c = 0; c < componentCount; ++c) {
            uint16_t nameLen = 0;
            if (!ReadU16(bytes, offset, nameLen) ||
                bytes.size() - offset < nameLen) {
                return false;
            }
            std::string name(bytes.begin() + offset,
                             bytes.begin() + offset + nameLen);
            offset += nameLen;
            uint32_t dataSize = 0;
            if (!ReadU32(bytes, offset, dataSize) ||
                bytes.size() - offset < dataSize ||
                !componentNames.insert(name).second) {
                return false;
            }
            const ComponentTypeInfo* info =
                m_ComponentManager->FindByName(name);
            if (info && info->triviallyCopyable && info->size == dataSize) {
                SavedComponent component{info, {}};
                component.data.assign(bytes.begin() + offset,
                                      bytes.begin() + offset + dataSize);
                saved.components.push_back(std::move(component));
            } else {
                STONEPOTATOES_LOG_WARNING(
                    "World: unknown/mismatched component '%s', skipping",
                    name.c_str());
            }
            offset += dataSize;
        }
        entities.push_back(std::move(saved));
    }

    if (offset != bytes.size()) {
        STONEPOTATOES_LOG_ERROR("World: trailing bytes in save file %s", path);
        return false;
    }

    // Mutate the world only after the complete file has passed validation.
    for (const SavedEntity& saved : entities) {
        Entity e = m_EntityManager->RestoreEntity(saved.index,
                                                  saved.generation);
        m_ArchetypeManager->EnsureRecord(e);
        for (const SavedComponent& component : saved.components) {
            m_ArchetypeManager->AddComponentData(
                e, component.info, component.data.data());
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
