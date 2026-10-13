#pragma once

#include "ResourceScatter.h"
#include "../../ai/NPC.h"   // access spec component types | 存取宣告所需
#include "../../ecs/World.h"
#include "../../ecs/system/System.h"
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// WorldStreamer — closes the loop between generative world content and
// living agents. Spectral-scattered resource nodes materialize as ECS
// entities while a chunk is resident (near any NPC) and are destroyed
// when it streams out. Harvested nodes never respawn: the world is
// generative, but consumption is permanent.
// WorldStreamer — 讓生成式世界內容與活動代理形成閉環。
// 頻譜散佈的資源節點在 chunk 載入時（任一 NPC 附近）實體化，
// 串出時銷毀。已被採集的節點不再重生 — 世界是生成的，
// 但消耗是永久的。
namespace StonePotatoes {
namespace World {

// Marker on every streamed node entity: origin identity + layer type.
// 每個串流節點實體上的標記：來源身分 + 圖層類型。
struct SpawnedNode {
    uint64_t nodeId = 0;
    uint32_t type = 0;
};

class WorldStreamer : public ECS::System {
public:
    struct Config {
        float chunkSize = 64.0f;
        int   radius = 1;             // chunks around each NPC | NPC 周圍 chunk 數
        float refreshInterval = 0.25f; // re-eval period (sim s) | 重估週期
        // Layer types that spawn with the AI::Food tag (NPCs can eat).
        // 會附加 AI::Food 標記的圖層類型（NPC 可食用）。
        std::unordered_set<uint32_t> edibleTypes = {1};
    };

    WorldStreamer(const ResourceScatter* scatter, Config cfg = {});

    void Initialize(ECS::World* world) override;
    void Update(ECS::World* world, float deltaTime) override;
    void DescribeAccess(ECS::AccessSpec& a) const override {
        // Reads NPC positions; chunk spawn/despawn goes through the
        // command buffer → not structural, can share a wave.
        // 只讀 NPC 位置；chunk 生成/卸載經指令緩衝區 → 非結構性，
        // 可與其他系統同波。
        a.Read<AI::NPCPosition>();
        a.Read<AI::Memory>();
    }

    size_t LoadedChunkCount() const { return m_Chunks.size(); }
    size_t SpawnedEntityCount() const;
    size_t HarvestedCount() const { return m_Harvested.size(); }

    // Mark a node consumed without unloading its chunk (e.g. external
    // harvester). Idempotent. | 不卸載 chunk 直接標記已採集（冪等）。
    void MarkHarvested(uint64_t nodeId) { m_Harvested.insert(nodeId); }

private:
    static int64_t ChunkKey(int32_t cx, int32_t cy) {
        return (static_cast<int64_t>(cx) << 32) |
               static_cast<uint32_t>(cy);
    }

    void LoadChunk(ECS::World* world, int32_t cx, int32_t cy);
    void UnloadChunk(ECS::World* world, int64_t key);

    const ResourceScatter* m_Scatter;
    Config m_Cfg;
    float m_Accum = 1e30f;  // force refresh on first tick | 首 tick 強制刷新
    // chunk key → spawned entities | chunk 鍵 → 已生成實體
    std::unordered_map<int64_t, std::vector<ECS::Entity>> m_Chunks;
    std::unordered_map<uint64_t, ECS::Entity> m_ByNode;  // nodeId → entity
    std::unordered_map<uint64_t, uint64_t> m_ByEntity;   // entity → nodeId
    std::unordered_set<uint64_t> m_Harvested;            // consumed forever
};

} // namespace World
} // namespace StonePotatoes
