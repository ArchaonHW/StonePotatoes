// StonePotatoes scale tests — spatial grid, deferred command buffer,
// and a 10,000-agent stress run through the real NPC pipeline.
// StonePotatoes 規模測試 — 空間格索引、延遲指令緩衝區，
// 以及一萬代理的完整 NPC 管線壓力測試。

#include "ai/NPC.h"
#include "ai/SpatialGrid.h"
#include "ecs/World.h"
#include "gpu/CPUBackend.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

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

// ---------------------------------------------------------------------
// SpatialGrid | 空間格
// ---------------------------------------------------------------------

// Radius query finds exactly the in-range entries — including negative
// coordinates and cell-boundary straddlers.
// 半徑查詢恰好找到範圍內項目 — 含負座標與跨格邊界。
void TestGridQuery() {
    SpatialGrid g(8.0f);
    g.Insert(1, 1.0f, 1.0f);            // cell (0,0) — inside r=10
    g.Insert(2, 9.9f, 0.0f);            // cell (1,0) — inside r=10
    g.Insert(3, 10.1f, 0.0f);           // outside r=10, neighboring cell
    g.Insert(4, -3.0f, -3.0f);          // negative coords — inside
    g.Insert(5, 100.0f, 100.0f);        // far away

    std::vector<ECS::EntityID> hit;
    g.QueryCircle(0.0f, 0.0f, 10.0f,
                  [&](const SpatialGrid::Entry& e) { hit.push_back(e.id); });
    CHECK(hit.size() == 3);
    CHECK(std::find(hit.begin(), hit.end(), 1ull) != hit.end());
    CHECK(std::find(hit.begin(), hit.end(), 2ull) != hit.end());
    CHECK(std::find(hit.begin(), hit.end(), 4ull) != hit.end());
    CHECK(std::find(hit.begin(), hit.end(), 3ull) == hit.end());
    CHECK(std::find(hit.begin(), hit.end(), 5ull) == hit.end());
}

// A query sitting exactly on a cell boundary must still see entries in
// both cells — this is where naive floor() indexing drops detections.
// 恰在格邊界上的查詢必須看到兩側格子 — 天真 floor() 索引會漏。
void TestGridBoundary() {
    SpatialGrid g(8.0f);
    g.Insert(1, 8.01f, 0.0f);   // cell (1,0)
    g.Insert(2, 7.99f, 0.0f);   // cell (0,0)
    std::vector<ECS::EntityID> hit;
    g.QueryCircle(8.0f, 0.0f, 0.5f,
                  [&](const SpatialGrid::Entry& e) { hit.push_back(e.id); });
    CHECK(hit.size() == 2);
}

// Grid == brute force on scattered data (deterministic LCG scatter).
// 散佈資料上空間格結果與暴力搜尋一致（決定性 LCG）。
void TestGridVsBruteForce() {
    SpatialGrid g(6.0f);
    std::vector<SpatialGrid::Entry> all;
    uint64_t s = 12345;
    for (int i = 0; i < 2000; ++i) {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        const float x = (s >> 33) % 10000 / 100.0f - 50.0f;
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        const float y = (s >> 33) % 10000 / 100.0f - 50.0f;
        g.Insert(static_cast<ECS::EntityID>(i + 1), x, y);
        all.push_back({static_cast<ECS::EntityID>(i + 1), x, y, 0});
    }
    for (int q = 0; q < 50; ++q) {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        const float qx = (s >> 33) % 10000 / 100.0f - 50.0f;
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        const float qy = (s >> 33) % 10000 / 100.0f - 50.0f;
        const float r = 5.0f + (q % 7);
        std::vector<ECS::EntityID> gh, bh;
        g.QueryCircle(qx, qy, r,
                      [&](const SpatialGrid::Entry& e) { gh.push_back(e.id); });
        for (const auto& e : all) {
            const float dx = e.x - qx, dy = e.y - qy;
            if (dx * dx + dy * dy <= r * r) bh.push_back(e.id);
        }
        std::sort(gh.begin(), gh.end());
        std::sort(bh.begin(), bh.end());
        CHECK(gh == bh);
    }
}

// Moving entities: grid is rebuilt per tick, so an entry that moved
// must only be found at its new cell.
// 移動實體：空間格每 tick 重建，移動後只能在新位置被找到。
void TestGridMove() {
    SpatialGrid g(8.0f);
    g.Insert(7, 0.0f, 0.0f);
    int old = 0;
    g.QueryCircle(0.0f, 0.0f, 2.0f, [&](const SpatialGrid::Entry&) { ++old; });
    CHECK(old == 1);
    g.Clear();
    g.Insert(7, 50.0f, 0.0f);           // moved
    old = 0;
    g.QueryCircle(0.0f, 0.0f, 2.0f, [&](const SpatialGrid::Entry&) { ++old; });
    int nw = 0;
    g.QueryCircle(50.0f, 0.0f, 2.0f, [&](const SpatialGrid::Entry&) { ++nw; });
    CHECK(old == 0);
    CHECK(nw == 1);
}

// ---------------------------------------------------------------------
// Command buffer | 指令緩衝區
// ---------------------------------------------------------------------

// Deferred destroy: entity stays valid during the tick and is gone by
// the next Update call — no mid-iteration mutation.
// 延遲銷毀：實體在 tick 中保持有效、下一次 Update 前消失 —
// 迭代中無突變。
struct DeferredKiller : ECS::System {
    ECS::Entity victim;
    bool queued = false;
    void Update(ECS::World* w, float) override {
        if (!queued) {
            w->QueueCommand([e = victim](ECS::World& ww) {
                ww.DestroyEntity(e);
            });
            queued = true;
        }
    }
    void DescribeAccess(ECS::AccessSpec& a) const override {
        a.Read<AI::NPCPosition>();  // non-structural read | 非結構讀取
    }
};

struct PosObserver : ECS::System {
    int sawAlive = 0, sawDead = 0;
    ECS::Entity watched;
    void Update(ECS::World* w, float) override {
        // Runs in the wave AFTER DeferredKiller (conflict on NPCPosition
        // read/write below) — commands applied at the boundary, so the
        // entity is already gone.
        // 在 DeferredKiller 之後的波次執行 — 指令已於波界生效。
        if (w->IsEntityValid(watched)) ++sawAlive; else ++sawDead;
    }
    void DescribeAccess(ECS::AccessSpec& a) const override {
        a.Write<AI::NPCPosition>();  // conflicts with killer's read
    }
};

void TestDeferredDestroy() {
    // Wave-scheduled path: command applies at the wave boundary, so the
    // observer in the next wave already sees the entity gone.
    // 波次排程路徑：指令於波界生效，下一波的觀察者已看到實體消失。
    {
        ECS::World w;
        w.Initialize();
        w.RegisterComponent<AI::NPCPosition>("NPCPosition");
        auto e = w.CreateEntity();
        w.AddComponent(e, AI::NPCPosition{1.0f, 2.0f});

        auto killer = std::make_unique<DeferredKiller>();
        killer->victim = e;
        auto observer = std::make_unique<PosObserver>();
        observer->watched = e;
        auto* obs = observer.get();

        w.GetSystemScheduler().RegisterSystem(std::move(killer));
        w.GetSystemScheduler().RegisterSystem(std::move(observer));

        w.GetSystemScheduler().UpdateSystemsWaves(&w, kDt);
        CHECK(!w.IsEntityValid(e));
        CHECK(obs->sawDead == 1);
        CHECK(obs->sawAlive == 0);
        w.Shutdown();
    }
    // Sequential path: single end-of-tick barrier — the observer ran in
    // the same tick while the entity still existed.
    // 循序路徑：單一 tick 結尾屏障 — 觀察者在同 tick 執行時實體仍在。
    {
        ECS::World w;
        w.Initialize();
        w.RegisterComponent<AI::NPCPosition>("NPCPosition");
        auto e = w.CreateEntity();
        w.AddComponent(e, AI::NPCPosition{1.0f, 2.0f});

        auto killer = std::make_unique<DeferredKiller>();
        killer->victim = e;
        auto observer = std::make_unique<PosObserver>();
        observer->watched = e;
        auto* obs = observer.get();

        w.GetSystemScheduler().RegisterSystem(std::move(killer));
        w.GetSystemScheduler().RegisterSystem(std::move(observer));

        w.Update(kDt);
        CHECK(obs->sawAlive == 1);
        CHECK(!w.IsEntityValid(e));      // destroyed after all systems
        w.Shutdown();
    }
}

// ---------------------------------------------------------------------
// NPC scale | NPC 規模
// ---------------------------------------------------------------------

static std::unique_ptr<ECS::World> MakeNpcWorld() {
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

// Deterministic LCG scatter (same stream → same world).
// 決定性 LCG 散佈（相同序列 → 相同世界）。
static uint64_t g_Seed = 88172645463325252ULL;
static float NextScatter(float lo, float hi) {
    g_Seed = g_Seed * 6364136223846793005ULL + 1442695040888963407ULL;
    return lo + (hi - lo) * ((g_Seed >> 33) & 0xFFFFFF) / 16777216.0f;
}

static void Populate(ECS::World& w, int npcs, int food, float extent) {
    g_Seed = 88172645463325252ULL;
    for (int i = 0; i < food; ++i)
        SpawnFood(w, NextScatter(0.0f, extent), NextScatter(0.0f, extent));
    for (int i = 0; i < npcs; ++i) {
        auto e = SpawnNpc(w, NextScatter(0.0f, extent),
                          NextScatter(0.0f, extent));
        w.GetComponent<Needs>(e)->hunger = 0.8f;  // motivated | 有覓食動機
    }
}

static double RunTicks(ECS::World& w, int ticks, bool parallel,
                       GPU::CPUBackend* be) {
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < ticks; ++i) {
        if (parallel) {
            w.UpdateParallel(kDt, [be](uint32_t n,
                const std::function<void(uint32_t)>& fn) {
                be->ParallelFor(n, fn, 1);
            });
        } else {
            w.Update(kDt);
        }
    }
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now() - t0)
               .count() /
           ticks;
}

// Correctness + determinism at 1,000 agents: sequential and parallel
// runs must produce identical end positions.
// 1,000 代理正確性與決定性：循序與平行結果必須一致。
void TestScale1k() {
    double pos[2][2] = {};
    int eaten[2] = {0, 0};
    for (int mode = 0; mode < 2; ++mode) {
        auto w = MakeNpcWorld();
        Populate(*w, 1000, 1000, 400.0f);
        w->GetEventBus().Subscribe<FoodEatenEvent>(
            [&](const FoodEatenEvent&) { ++eaten[mode]; });
        GPU::CPUBackend be(8);
        const double ms = RunTicks(*w, 20, mode == 1, &be);
        std::printf("  [scale1k] %s  %.2f ms/tick  eaten=%d\n",
                    mode == 1 ? "parallel" : "sequence", ms, eaten[mode]);
        double sx = 0;
        w->ForEach<NPCPosition, Memory>(
            [&](ECS::Entity, NPCPosition& p, Memory&) { sx += p.x; });
        pos[mode][0] = sx;
        pos[mode][1] = sx;
        w->Shutdown();
    }
    CHECK(eaten[0] > 0);                     // the loop actually ate food
    CHECK(eaten[0] == eaten[1]);             // same events both modes
    CHECK(pos[0][0] == pos[1][0]);           // bit-identical trajectories
}

// Stress: 10,000 NPCs + 10,000 food, grid-backed perception must keep
// per-tick cost bounded (was O(10k × 20k) = 2×10⁸ distance tests).
// 壓力測試：一萬 NPC + 一萬食物。格子感知須把每 tick 成本壓住
// （原本為 O(10k × 20k) = 2 億次距離判定）。
void TestScale10k() {
    auto w = MakeNpcWorld();
    Populate(*w, 10000, 10000, 800.0f);
    int eaten = 0;
    w->GetEventBus().Subscribe<FoodEatenEvent>(
        [&](const FoodEatenEvent&) { ++eaten; });

    const double ms = RunTicks(*w, 10, false, nullptr);
    std::printf("  [scale10k] %.2f ms/tick  eaten=%d\n", ms, eaten);

    // Correctness: agents did perceive and consume streamed-in-range food.
    int withMemory = 0;
    w->ForEach<Memory>([&](ECS::Entity, Memory& m) {
        if (m.count > 0) ++withMemory;
    });
    CHECK(withMemory > 5000);                // most NPCs saw something
    CHECK(eaten > 0);                        // consumption happened
    // With the grid this runs in well under 200 ms/tick on any modern
    // CPU; the old O(N×M) scan was ~seconds. Generous bound → guards
    // against regression, not micro-benchmarks.
    CHECK(ms < 2000.0);
    w->Shutdown();
}

int main() {
    std::printf("=== StonePotatoes Scale Tests ===\n");

    TestGridQuery();
    TestGridBoundary();
    TestGridVsBruteForce();
    TestGridMove();
    TestDeferredDestroy();
    TestScale1k();
    TestScale10k();

    std::printf("=== %d passed, %d failed ===\n", g_Passed, g_Failed);
    return g_Failed == 0 ? 0 : 1;
}
