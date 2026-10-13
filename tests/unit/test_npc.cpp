// StonePotatoes MVP NPC vertical-slice tests — local-information agents.
// StonePotatoes MVP NPC 垂直切片測試 — 局部資訊代理。
//
// Every scenario is deterministic: fixed dt, deterministic wander hash.
// 所有場景皆為決定性：固定 dt、決定性遊蕩雜湊。

#include "ai/NPC.h"
#include "ecs/World.h"

#include <cmath>
#include <cstdio>
#include <memory>

using namespace StonePotatoes;
using namespace StonePotatoes::AI;

static int g_Passed = 0;
static int g_Failed = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (cond) { ++g_Passed; }                                          \
        else {                                                             \
            ++g_Failed;                                                    \
            std::printf("[FAIL] %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
        }                                                                  \
    } while (0)

static constexpr float kDt = 0.1f;

// World with the full NPC pipeline registered in order.
// 以順序註冊完整 NPC 管線的世界。
static std::unique_ptr<ECS::World> MakeWorld() {
    auto w = std::make_unique<ECS::World>();
    w->Initialize();
    RegisterNpcComponents(*w);
    auto& s = w->GetSystemScheduler();
    s.RegisterSystem(std::make_unique<PerceptionSystem>());
    s.RegisterSystem(std::make_unique<DecisionSystem>());
    s.RegisterSystem(std::make_unique<ActionSystem>());
    s.RegisterSystem(std::make_unique<NpcMovementSystem>());
    return w;
}

static ECS::Entity SpawnFood(ECS::World& w, float x, float y) {
    ECS::Entity e = w.CreateEntity();
    w.AddComponent(e, NPCPosition{x, y});
    w.AddComponent(e, Food{});
    return e;
}

static ECS::Entity SpawnThreat(ECS::World& w, float x, float y) {
    ECS::Entity e = w.CreateEntity();
    w.AddComponent(e, NPCPosition{x, y});
    w.AddComponent(e, Threat{});
    return e;
}

static void Tick(ECS::World& w, int n) {
    for (int i = 0; i < n; ++i) w.Update(kDt);
}

static const MemoryRecord* FindRecord(const Memory& m, uint64_t eid) {
    const uint32_t i = m.Find(eid);
    return i == Memory::kMax ? nullptr : &m.records[i];
}

// --- Locality: sight radius gates knowledge | 視界限制知識 ---
void TestLocality() {
    auto w = MakeWorld();
    ECS::Entity npc = SpawnNpc(*w, 0.0f, 0.0f);
    ECS::Entity nearFood = SpawnFood(*w, 4.0f, 0.0f);   // inside radius 10
    ECS::Entity farFood = SpawnFood(*w, 50.0f, 0.0f);   // outside

    Tick(*w, 1);

    const Memory* mem = w->GetComponent<Memory>(npc);
    CHECK(mem != nullptr);
    CHECK(FindRecord(*mem, nearFood.id) != nullptr);    // saw near food
    CHECK(FindRecord(*mem, farFood.id) == nullptr);     // far food unknown
}

// --- Seek: hungry NPC walks to remembered food and eats | 覓食與進食 ---
void TestSeekAndEat() {
    auto w = MakeWorld();
    ECS::Entity npc = SpawnNpc(*w, 0.0f, 0.0f);
    w->GetComponent<Needs>(npc)->hunger = 0.9f;
    ECS::Entity food = SpawnFood(*w, 6.0f, 0.0f);

    int ateFired = 0;
    w->GetEventBus().Subscribe<FoodEatenEvent>(
        [&](const FoodEatenEvent& e) {
            ++ateFired;
            CHECK(e.eater == npc);
            CHECK(e.food == food);
        });

    const float d0 = std::fabs(6.0f - w->GetComponent<NPCPosition>(npc)->x);
    Tick(*w, 10);
    const float d1 = std::fabs(6.0f - w->GetComponent<NPCPosition>(npc)->x);
    CHECK(d1 < d0);                                     // approaching
    CHECK(w->GetComponent<Decision>(npc)->action == Action::SeekFood);

    Tick(*w, 30);  // enough to arrive; World::Update dispatches events
    CHECK(ateFired == 1);                               // eaten exactly once
    CHECK(!w->IsEntityValid(food));                     // food consumed
    // Hunger resets to 0 on eating, then climbs again (rate 0.05/s).
    // 進食時歸零，之後再上升。
    CHECK(w->GetComponent<Needs>(npc)->hunger < 0.3f);
}

// --- Flee: remembered threat overrides hunger | 逃跑優先於覓食 ---
void TestFlee() {
    auto w = MakeWorld();
    ECS::Entity npc = SpawnNpc(*w, 0.0f, 0.0f);
    w->GetComponent<Needs>(npc)->hunger = 0.9f;
    SpawnFood(*w, 5.0f, 0.0f);
    ECS::Entity wolf = SpawnThreat(*w, 2.0f, 0.0f);

    Tick(*w, 1);
    const Decision* d = w->GetComponent<Decision>(npc);
    CHECK(d->action == Action::Flee);
    CHECK(d->target == wolf.id);

    const float x0 = w->GetComponent<NPCPosition>(npc)->x;
    Tick(*w, 10);
    const float x1 = w->GetComponent<NPCPosition>(npc)->x;
    CHECK(x1 < x0);                                     // ran away (-x)
}

// --- Wander: no needs, no memories → deterministic drift | 遊蕩 ---
void TestWander() {
    auto w = MakeWorld();
    ECS::Entity npc = SpawnNpc(*w, 0.0f, 0.0f);
    w->GetComponent<Needs>(npc)->hunger = 0.0f;

    Tick(*w, 1);
    CHECK(w->GetComponent<Decision>(npc)->action == Action::Wander);

    Tick(*w, 5);
    const NPCPosition* p = w->GetComponent<NPCPosition>(npc);
    CHECK(std::fabs(p->x) > 0.01f || std::fabs(p->y) > 0.01f);
}

// --- Stale memory: food eaten by rival → walk there, then forget ---
// 過期記憶：食物被對手吃掉 → 走到位置後遺忘
void TestStaleMemory() {
    auto w = MakeWorld();
    ECS::Entity rival = SpawnNpc(*w, 5.5f, 0.0f);
    w->GetComponent<Needs>(rival)->hunger = 0.9f;
    ECS::Entity npc = SpawnNpc(*w, -20.0f, 0.0f);
    w->GetComponent<Needs>(npc)->hunger = 0.9f;
    ECS::Entity food = SpawnFood(*w, 6.0f, 0.0f);
    (void)food;

    Tick(*w, 20);  // rival eats it; npc saw nothing yet (too far)
    w->GetEventBus().Dispatch();
    CHECK(!w->IsEntityValid(food));

    // Teleport npc next to the food's last position — now it sees the
    // spot, and Perception will find... nothing there.
    // 將 npc 傳送到食物位置旁 — 它「看到」那裡，但已無食物。
    w->GetComponent<NPCPosition>(npc)->x = 4.0f;
    Tick(*w, 1);
    // It never saw the food (dead before first sight) → no record →
    // falls to Wander, not SeekFood.
    CHECK(FindRecord(*w->GetComponent<Memory>(npc), food.id) == nullptr);
    CHECK(w->GetComponent<Decision>(npc)->action != Action::SeekFood);
}

// --- Determinism: identical runs → identical trajectories | 決定性 ---
void TestDeterminism() {
    float px[2] = {0, 0}, py[2] = {0, 0};
    for (int run = 0; run < 2; ++run) {
        auto w = MakeWorld();
        ECS::Entity npc = SpawnNpc(*w, 1.0f, 2.0f);
        Tick(*w, 40);
        px[run] = w->GetComponent<NPCPosition>(npc)->x;
        py[run] = w->GetComponent<NPCPosition>(npc)->y;
    }
    CHECK(px[0] == px[1] && py[0] == py[1]);
}

// --- Memory forget: leave radius long enough → record expires ---
// 記憶遺忘：離開視野足夠久 → 記錄過期
void TestForget() {
    auto w = MakeWorld();
    ECS::Entity npc = SpawnNpc(*w, 0.0f, 0.0f);
    w->GetComponent<Perception>(npc)->forgetAfter = 1.0f;
    ECS::Entity food = SpawnFood(*w, 4.0f, 0.0f);

    Tick(*w, 1);
    CHECK(FindRecord(*w->GetComponent<Memory>(npc), food.id) != nullptr);

    // Move the food out of sight; after forgetAfter the record expires.
    // 食物移出視野；forgetAfter 後記錄消失。
    w->GetComponent<NPCPosition>(food)->x = 90.0f;
    Tick(*w, 12);                                       // 1.2 s > 1.0 s
    CHECK(FindRecord(*w->GetComponent<Memory>(npc), food.id) == nullptr);
}

int main() {
    std::printf("=== StonePotatoes NPC Vertical-Slice Tests ===\n");

    TestLocality();
    TestSeekAndEat();
    TestFlee();
    TestWander();
    TestStaleMemory();
    TestDeterminism();
    TestForget();

    std::printf("=== %d passed, %d failed ===\n", g_Passed, g_Failed);
    return g_Failed == 0 ? 0 : 1;
}
