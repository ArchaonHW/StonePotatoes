#include "NPC.h"

#include "../core/logging/Logger.h"

#include <cmath>
#include <vector>

namespace StonePotatoes {
namespace AI {

namespace {
constexpr float kTwoPiF = 6.283185307179586f;

float Dist2(float ax, float ay, float bx, float by) {
    const float dx = bx - ax, dy = by - ay;
    return dx * dx + dy * dy;
}

// Deterministic hash → [0, 2π). Same (id, bucket) → same heading.
// 決定性雜湊 → [0,2π)：相同 (id, bucket) → 相同航向。
uint64_t Hash64(uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}
} // namespace

// ------------------------------------------------------------------
// PerceptionSystem | 感知系統
// ------------------------------------------------------------------
void PerceptionSystem::Update(ECS::World* world, float deltaTime) {
    m_Time += deltaTime;
    const float now = m_Time;

    // Publish sim time so DecisionSystem can bucket deterministically.
    // 發佈模擬時間，供 DecisionSystem 做決定性時間分桶。
    auto& rm = world->GetResourceManager();
    if (!rm.Has<SimClock>()) rm.Insert<SimClock>();
    rm.Get<SimClock>()->time = now;

    // Snapshot all perceivable things once per tick (positions only —
    // no hidden state leaks into memory).
    // 每 tick 快照所有可感知物（僅位置 — 隱藏狀態不流入記憶）。
    struct Seen { ECS::Entity e; uint8_t kind; float x, y; };
    std::vector<Seen> candidates;
    world->ForEach<NPCPosition, Food>(
        [&](ECS::Entity e, NPCPosition& p, Food&) {
            candidates.push_back({e, 1, p.x, p.y});
        });
    world->ForEach<NPCPosition, Threat>(
        [&](ECS::Entity e, NPCPosition& p, Threat&) {
            candidates.push_back({e, 2, p.x, p.y});
        });

    world->ForEach<NPCPosition, Perception, Memory>(
        [&](ECS::Entity self, NPCPosition& p, Perception& per, Memory& mem) {
            const float r2 = per.radius * per.radius;
            for (const Seen& s : candidates) {
                if (s.e == self) continue;
                if (Dist2(p.x, p.y, s.x, s.y) > r2) continue;  // 視野外
                uint32_t idx = mem.Find(s.e.id);
                if (idx == Memory::kMax) {
                    if (mem.count < Memory::kMax) {
                        idx = mem.count++;
                    } else {
                        // Full: overwrite the stalest record.
                        // 已滿：覆寫最久未見的記錄。
                        idx = 0;
                        for (uint32_t i = 1; i < mem.count; ++i)
                            if (mem.records[i].lastSeen <
                                mem.records[idx].lastSeen) idx = i;
                    }
                }
                mem.records[idx] = {s.e.id, s.kind, s.x, s.y, now};
            }
            // Forget stale records (swap-remove, keep POD layout).
            // 遺忘過期記錄（交換移除，維持 POD 佈局）。
            for (uint32_t i = 0; i < mem.count;) {
                if (now - mem.records[i].lastSeen > per.forgetAfter) {
                    mem.records[i] = mem.records[--mem.count];
                } else {
                    ++i;
                }
            }
        });
}

// ------------------------------------------------------------------
// DecisionSystem | 決策系統
// ------------------------------------------------------------------
void DecisionSystem::Update(ECS::World* world, float deltaTime) {
    (void)deltaTime;
    // Wander re-picks every ~2 s of sim time. Bucket comes from the
    // lastSeen clock the agent itself owns — no external time reads.
    // 遊蕩每 ~2 秒重選方向；時間桶取自代理自身時鐘。
    const auto* clock = world->GetResourceManager().Get<SimClock>();
    const float now = clock ? clock->time : 0.0f;

    world->ForEach<NPCPosition, Memory, Needs, Decision, NpcStats>(
        [&](ECS::Entity self, NPCPosition& p, Memory& mem, Needs& needs,
            Decision& d, NpcStats& st) {
            // 1) Flee: any remembered threat within fleeRadius.
            // 1) 逃跑：記憶中 fleeRadius 內有威脅。
            const MemoryRecord* threat = nullptr;
            float best = st.fleeRadius * st.fleeRadius;
            for (uint32_t i = 0; i < mem.count; ++i) {
                const MemoryRecord& r = mem.records[i];
                if (r.kind != 2) continue;
                const float d2 = Dist2(p.x, p.y, r.x, r.y);
                if (d2 < best) { best = d2; threat = &r; }
            }
            if (threat) {
                d.action = Action::Flee;
                d.targetX = threat->x;
                d.targetY = threat->y;
                d.target = threat->entity;
                return;
            }

            // 2) Seek: hungry + remembered food → nearest record.
            // 2) 覓食：餓且記得食物 → 最近的記錄。
            if (needs.hunger >= needs.seekThreshold) {
                const MemoryRecord* food = nullptr;
                float bd = 1e30f;
                for (uint32_t i = 0; i < mem.count; ++i) {
                    const MemoryRecord& r = mem.records[i];
                    if (r.kind != 1) continue;
                    const float d2 = Dist2(p.x, p.y, r.x, r.y);
                    if (d2 < bd) { bd = d2; food = &r; }
                }
                if (food) {
                    d.action = Action::SeekFood;
                    d.targetX = food->x;
                    d.targetY = food->y;
                    d.target = food->entity;
                    return;
                }
            }

            // 3) Wander: deterministic heading, re-picked per 2s bucket.
            // 3) 遊蕩：決定性航向，每 2 秒桶重選。
            const uint64_t bucket =
                static_cast<uint64_t>(now * 0.5f);
            const float ang =
                (Hash64(self.id * 2654435761ULL ^ bucket) & 0xFFFFFF) /
                16777216.0f * kTwoPiF;
            d.action = Action::Wander;
            d.targetX = p.x + std::cos(ang);
            d.targetY = p.y + std::sin(ang);
            d.target = 0;
        });
}

// ------------------------------------------------------------------
// ActionSystem | 行動系統
// ------------------------------------------------------------------
void ActionSystem::Update(ECS::World* world, float deltaTime) {
    std::vector<ECS::Entity> eaten;   // destroy after iteration | 迭代後銷毀
    std::vector<ECS::Entity> eaters;

    world->ForEach<NPCPosition, NPCVelocity, Needs, Decision, NpcStats,
                   Memory>(
        [&](ECS::Entity self, NPCPosition& p, NPCVelocity& v, Needs& needs,
            Decision& d, NpcStats& st, Memory& mem) {
            needs.hunger += needs.hungerRate * deltaTime;
            if (needs.hunger > 1.0f) needs.hunger = 1.0f;

            float dx = d.targetX - p.x, dy = d.targetY - p.y;
            const float len = std::sqrt(dx * dx + dy * dy);

            switch (d.action) {
            case Action::Flee:
                // Away from remembered threat position.
                // 背離記憶中的威脅位置。
                if (len > 1e-4f) {
                    v.x = -dx / len * st.speed;
                    v.y = -dy / len * st.speed;
                }
                break;
            case Action::SeekFood:
                if (len <= st.eatDistance) {
                    if (world->IsEntityValid(ECS::Entity(d.target))) {
                        eaten.push_back(ECS::Entity(d.target));
                        eaters.push_back(self);
                    } else {
                        // Someone ate it first — forget the stale record.
                        // 被別人先吃掉了 — 忘掉這筆過期記憶。
                        const uint32_t idx = mem.Find(d.target);
                        if (idx != Memory::kMax)
                            mem.records[idx] = mem.records[--mem.count];
                    }
                    v.x = v.y = 0.0f;
                } else {
                    v.x = dx / len * st.speed;
                    v.y = dy / len * st.speed;
                }
                break;
            case Action::Wander:
                v.x = dx * st.speed;  // target is 1 unit away
                v.y = dy * st.speed;
                break;
            default:
                v.x = v.y = 0.0f;
                break;
            }
        });

    // Deferred structural change: destroy eaten food, feed the eater.
    // 延遲結構變更：銷毀被吃掉的食物、餵飽進食者。
    for (size_t i = 0; i < eaten.size(); ++i) {
        world->GetEventBus().Queue(
            FoodEatenEvent{eaters[i], eaten[i]});
        world->DestroyEntity(eaten[i]);
        if (Needs* n = world->GetComponent<Needs>(eaters[i]))
            n->hunger = 0.0f;
    }
}

// ------------------------------------------------------------------
// NpcMovementSystem | NPC 移動系統
// ------------------------------------------------------------------
void NpcMovementSystem::Update(ECS::World* world, float deltaTime) {
    world->ForEach<NPCPosition, NPCVelocity>(
        [deltaTime](ECS::Entity, NPCPosition& p, NPCVelocity& v) {
            p.x += v.x * deltaTime;
            p.y += v.y * deltaTime;
        });
}

// ------------------------------------------------------------------
// Registration / spawn | 註冊／生成
// ------------------------------------------------------------------
void RegisterNpcComponents(ECS::World& world) {
    world.RegisterComponent<NPCPosition>("NPCPosition");
    world.RegisterComponent<NPCVelocity>("NPCVelocity");
    world.RegisterComponent<Perception>("Perception");
    world.RegisterComponent<Memory>("Memory");
    world.RegisterComponent<Needs>("Needs");
    world.RegisterComponent<Decision>("Decision");
    world.RegisterComponent<NpcStats>("NpcStats");
    world.RegisterComponent<Food>("Food");
    world.RegisterComponent<Threat>("Threat");
}

ECS::Entity SpawnNpc(ECS::World& world, float x, float y) {
    ECS::Entity e = world.CreateEntity();
    world.AddComponent(e, NPCPosition{x, y});
    world.AddComponent(e, NPCVelocity{});
    world.AddComponent(e, Perception{});
    world.AddComponent(e, Memory{});
    world.AddComponent(e, Needs{});
    world.AddComponent(e, Decision{});
    world.AddComponent(e, NpcStats{});
    return e;
}

} // namespace AI
} // namespace StonePotatoes
