#include "WorldStreamer.h"

#include "../../ai/NPC.h"
#include "../../core/logging/Logger.h"

#include <cmath>

namespace StonePotatoes {
namespace World {

WorldStreamer::WorldStreamer(const ResourceScatter* scatter, Config cfg)
    : m_Scatter(scatter), m_Cfg(std::move(cfg)) {}

void WorldStreamer::Initialize(ECS::World* world) {
    world->RegisterComponent<SpawnedNode>("SpawnedNode");
    // Harvested bitset hook: when an NPC eats a streamed food entity,
    // record its stable node id so it never respawns.
    // 採集位元集掛鉤：NPC 吃掉串流食物時記下穩定節點 id，永不重生。
    world->GetEventBus().Subscribe<AI::FoodEatenEvent>(
        [this](const AI::FoodEatenEvent& e) {
            auto it = m_ByEntity.find(e.food.id);
            if (it != m_ByEntity.end()) {
                m_Harvested.insert(it->second);
                m_ByNode.erase(it->second);
                m_ByEntity.erase(it);
            }
        });
}

size_t WorldStreamer::SpawnedEntityCount() const {
    size_t n = 0;
    for (const auto& kv : m_Chunks) n += kv.second.size();
    return n;
}

void WorldStreamer::LoadChunk(ECS::World* world, int32_t cx, int32_t cy) {
    const int64_t key = ChunkKey(cx, cy);
    std::vector<ECS::Entity> spawned;
    const float c = m_Cfg.chunkSize;
    for (const ResourceNode& node :
         m_Scatter->GenerateChunk(cx, cy, c)) {
        if (m_Harvested.count(node.id)) continue;   // consumed forever
        ECS::Entity e = world->CreateEntity();
        world->AddComponent(e, AI::NPCPosition{node.x, node.y});
        world->AddComponent(e, SpawnedNode{node.id, node.type});
        if (m_Cfg.edibleTypes.count(node.type))
            world->AddComponent(e, AI::Food{});
        spawned.push_back(e);
        m_ByNode[node.id] = e;
        m_ByEntity[e.id] = node.id;
    }
    m_Chunks[key] = std::move(spawned);
}

void WorldStreamer::UnloadChunk(ECS::World* world, int64_t key) {
    auto it = m_Chunks.find(key);
    if (it == m_Chunks.end()) return;
    for (ECS::Entity e : it->second) {
        auto ne = m_ByEntity.find(e.id);
        if (ne != m_ByEntity.end()) {
            m_ByNode.erase(ne->second);
            m_ByEntity.erase(ne);
        }
        world->DestroyEntity(e);
    }
    m_Chunks.erase(it);
}

void WorldStreamer::Update(ECS::World* world, float deltaTime) {
    m_Accum += deltaTime;
    if (m_Accum < m_Cfg.refreshInterval) return;
    m_Accum = 0.0f;

    // Required chunks: radius around every NPC position.
    // 需求 chunk：每個 NPC 位置周圍 radius 格。
    std::unordered_set<int64_t> needed;
    const float cs = m_Cfg.chunkSize;
    world->ForEach<AI::NPCPosition, AI::Memory>(
        [&](ECS::Entity, AI::NPCPosition& p, AI::Memory&) {
            const int32_t ccx =
                static_cast<int32_t>(std::floor(p.x / cs));
            const int32_t ccy =
                static_cast<int32_t>(std::floor(p.y / cs));
            for (int dy = -m_Cfg.radius; dy <= m_Cfg.radius; ++dy)
                for (int dx = -m_Cfg.radius; dx <= m_Cfg.radius; ++dx)
                    needed.insert(ChunkKey(ccx + dx, ccy + dy));
        });

    // Unload chunks that fell out of range. Deferred: actual entity
    // destruction runs at the wave boundary, so this system is not a
    // serial structural barrier.
    // 卸載脫離範圍的 chunk。延遲執行：實體銷毀在波次邊界進行，
    // 本系統因此不再是序列結構屏障。
    std::vector<int64_t> drop;
    for (const auto& kv : m_Chunks)
        if (!needed.count(kv.first)) drop.push_back(kv.first);
    for (int64_t key : drop)
        world->QueueCommand([this, key](ECS::World& w) {
            UnloadChunk(&w, key);
        });

    // Load newly needed chunks (deferred as well). Chunk bookkeeping
    // keys (m_Chunks) are only touched inside the commands, which run
    // single-threaded at the wave boundary → no data race.
    // 載入新需要的 chunk（同樣延遲）。m_Chunks 等簿記只在指令內
    // 觸碰，而指令在波次邊界單執行緒執行 → 無資料競爭。
    for (int64_t key : needed) {
        if (m_Chunks.count(key)) continue;
        const int32_t cx = static_cast<int32_t>(key >> 32);
        const int32_t cy = static_cast<int32_t>(
            static_cast<uint32_t>(key & 0xFFFFFFFF));
        world->QueueCommand([this, cx, cy](ECS::World& w) {
            LoadChunk(&w, cx, cy);
        });
    }
}

} // namespace World
} // namespace StonePotatoes
