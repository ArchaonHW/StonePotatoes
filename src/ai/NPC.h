#pragma once

#include "../ecs/World.h"
#include "../ecs/system/System.h"
#include <cstdint>

// MVP NPC vertical slice — local-information agents.
// NPCs decide from what they have SEEN (Memory), never from global
// state. Pipeline per tick: Perception → Decision → Action → Movement.
// MVP NPC 垂直切片 — 局部資訊代理。
// NPC 僅依據「看過的事物」（Memory）決策，永不讀取全域狀態。
// 每 tick 管線：感知 → 決策 → 行動 → 移動。
namespace StonePotatoes {
namespace AI {

// --- Components (POD, serializable) | 組件（POD，可序列化） ---

struct NPCPosition { float x = 0, y = 0; };
struct NPCVelocity { float x = 0, y = 0; };

// What an agent can sense | 代理可感知的範圍
struct Perception {
    float radius = 10.0f;      // sight radius | 視野半徑
    float forgetAfter = 30.0f; // memory decay (sim seconds) | 記憶衰退時間
};

// What the world provides | 世界提供的標記
struct Food {};    // edible | 可食
struct Threat {};  // dangerous | 危險

// Fixed-capacity memory — deliberately small: agents can't know
// everything. Kinds: 1=Food, 2=Threat.
// 固定容量記憶 — 刻意設小：代理不可能什麼都知道。
// 類別：1=食物，2=威脅。
struct MemoryRecord {
    uint64_t entity = 0;   // ECS::EntityID
    uint8_t  kind = 0;
    float x = 0, y = 0;    // position when last seen | 最後看見的位置
    float lastSeen = 0;    // sim time | 模擬時間
};
struct Memory {
    static constexpr uint32_t kMax = 16;
    MemoryRecord records[kMax];
    uint32_t count = 0;

    // Returns index or kMax | 找不到時回傳 kMax
    uint32_t Find(uint64_t entity) const {
        for (uint32_t i = 0; i < count; ++i)
            if (records[i].entity == entity) return i;
        return kMax;
    }
};

// Drives | 內在需求
struct Needs {
    float hunger = 0.0f;       // 0=satiated, 1=starving | 0=飽，1=餓
    float hungerRate = 0.05f;  // per sim-second | 每模擬秒
    float seekThreshold = 0.5f;
};

enum class Action : uint8_t { Idle, Wander, SeekFood, Flee };

// Chosen intent — target is the REMEMBERED position, not live state.
// 選定意圖 — 目標是「記憶中的位置」，非即時狀態。
struct Decision {
    Action action = Action::Idle;
    float targetX = 0, targetY = 0;
    uint64_t target = 0;  // remembered entity | 記得的目標實體
};

struct NpcStats {
    float speed = 3.0f;
    float fleeRadius = 8.0f;
    float eatDistance = 0.6f;
};

// --- Events | 事件 ---
struct FoodEatenEvent {
    ECS::Entity eater;
    ECS::Entity food;
};

// World resource: simulation clock shared by the AI systems.
// 世界資源：AI 系統共用的模擬時鐘。
struct SimClock { float time = 0.0f; };

// --- Systems | 系統 ---

// Writes Memory from entities inside Perception.radius. This is the
// ONLY path world knowledge enters the agent — outside radius the
// entity effectively does not exist for it.
// 將視野內實體寫入記憶。這是世界資訊進入代理的唯一管道 —
// 視野外實體對它而言不存在。
class PerceptionSystem : public ECS::System {
public:
    void Update(ECS::World* world, float deltaTime) override;
    void DescribeAccess(ECS::AccessSpec& a) const override {
        a.Read<NPCPosition>();
        a.Read<Food>();
        a.Read<Threat>();
        a.Write<Memory>();
    }
    float Time() const { return m_Time; }
private:
    float m_Time = 0.0f;
};

// Picks an Action from Memory + Needs. Flee > Seek > Wander.
// Deterministic wander direction from (entity, time bucket).
// 依記憶與需求選擇行動：逃跑 > 覓食 > 遊蕩。
// 遊蕩方向由 (實體, 時間桶) 決定性決定。
class DecisionSystem : public ECS::System {
public:
    void Update(ECS::World* world, float deltaTime) override;
    void DescribeAccess(ECS::AccessSpec& a) const override {
        a.Read<NPCPosition>();
        a.Read<Memory>();
        a.Read<Needs>();
        a.Write<Decision>();
    }
};

// Decision → Velocity; eats food on arrival (deferred destroy +
// event). Hunger grows over time, resets when food is consumed.
// 決策 → 速度；抵達時進食（延遲銷毀 + 事件）。
// 飢餓隨時間上升，進食後重置。
class ActionSystem : public ECS::System {
public:
    void Update(ECS::World* world, float deltaTime) override;
    void DescribeAccess(ECS::AccessSpec& a) const override {
        a.Read<NPCPosition>();
        a.Read<Decision>();
        a.Read<NpcStats>();
        a.Write<NPCVelocity>();
        a.Write<Needs>();
        a.Write<Memory>();
        // Eaten food is destroyed via the command buffer — no longer
        // structural. 進食銷毀改經指令緩衝區 — 不再是結構性系統。
    }
};

// Integrates position from velocity.
// 以速度積分位置。
class NpcMovementSystem : public ECS::System {
public:
    void Update(ECS::World* world, float deltaTime) override;
    void DescribeAccess(ECS::AccessSpec& a) const override {
        a.Read<NPCVelocity>();
        a.Write<NPCPosition>();
    }
};

// Registers all NPC component types with stable names.
// 以穩定名稱註冊所有 NPC 組件型別。
void RegisterNpcComponents(ECS::World& world);

// Convenience: a wired-up NPC entity.
// 便捷函式：建立一個完整的 NPC 實體。
ECS::Entity SpawnNpc(ECS::World& world, float x, float y);

} // namespace AI
} // namespace StonePotatoes
