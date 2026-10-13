// StonePotatoes ECS parallel scheduler tests — wave formation,
// ordering, determinism, and real parallel execution via IQGPU's
// CPU backend.
// StonePotatoes ECS 平行排程測試 — 波次形成、排序、
// 決定性，以及透過 IQGPU CPU 後端的真實平行執行。

#include "ecs/World.h"
#include "gpu/CPUBackend.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <thread>

using namespace StonePotatoes;

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

struct CompA { float v = 0; };
struct CompB { float v = 0; };

// --- Test systems with declared access | 已宣告存取的測試系統 ---

struct WriteA : ECS::System {
    void Update(ECS::World* w, float) override {
        w->ForEach<CompA>([](ECS::Entity, CompA& a) { a.v += 1.0f; });
    }
    void DescribeAccess(ECS::AccessSpec& a) const override {
        a.Write<CompA>();
    }
};

struct WriteB : ECS::System {
    void Update(ECS::World* w, float) override {
        w->ForEach<CompB>([](ECS::Entity, CompB& b) { b.v += 2.0f; });
    }
    void DescribeAccess(ECS::AccessSpec& a) const override {
        a.Write<CompB>();
    }
};

// Reads A, writes B — must run after WriteA, conflicts with WriteB.
// 讀 A 寫 B — 必須在 WriteA 之後，與 WriteB 衝突。
struct CopyAToB : ECS::System {
    void Update(ECS::World* w, float) override {
        w->ForEach<CompA, CompB>(
            [](ECS::Entity, CompA& a, CompB& b) { b.v = a.v * 10.0f; });
    }
    void DescribeAccess(ECS::AccessSpec& a) const override {
        a.Read<CompA>();
        a.Write<CompB>();
    }
};

// Legacy system: declares nothing → serial barrier.
// 舊式系統：未宣告 → 序列屏障。
struct LegacyBarrier : ECS::System {
    int ran = 0;
    void Update(ECS::World*, float) override { ++ran; }
};

static ECS::ParallelForFn BackendExecutor(GPU::CPUBackend& be) {
    return [&be](uint32_t n, const std::function<void(uint32_t)>& fn) {
        be.ParallelFor(n, fn, 1);
    };
}

// --- Wave formation: disjoint writes → same wave | 波次形成 ---
void TestWaveFormation() {
    ECS::World w;
    w.Initialize();
    w.GetSystemScheduler().RegisterSystem(std::make_unique<WriteA>());
    w.GetSystemScheduler().RegisterSystem(std::make_unique<WriteB>());

    auto st = w.GetSystemScheduler().UpdateSystemsWaves(&w, 0.016f);
    CHECK(st.waves == 1);          // independent → single parallel wave
    CHECK(st.maxWaveSize == 2);
    CHECK(st.systemsRun == 2);
    w.Shutdown();
}

// --- Ordering: conflicting systems keep registration order ---
// 排序：衝突系統維持註冊順序
void TestOrdering() {
    ECS::World w;
    w.Initialize();
    w.RegisterComponent<CompA>("CompA");
    w.RegisterComponent<CompB>("CompB");
    auto e = w.CreateEntity();
    w.AddComponent(e, CompA{});
    w.AddComponent(e, CompB{});

    w.GetSystemScheduler().RegisterSystem(std::make_unique<WriteA>());
    w.GetSystemScheduler().RegisterSystem(std::make_unique<CopyAToB>());
    w.GetSystemScheduler().RegisterSystem(std::make_unique<WriteB>());

    auto st = w.GetSystemScheduler().UpdateSystemsWaves(&w, 0.016f);
    // WriteA|CopyAToB conflict (A read-after-write); CopyAToB|WriteB
    // conflict (B write-write) → three ordered waves.
    // WriteA|CopyAToB 衝突；CopyAToB|WriteB 衝突 → 三個有序波次。
    CHECK(st.waves == 3);
    CHECK(w.GetComponent<CompA>(e)->v == 1.0f);
    CHECK(w.GetComponent<CompB>(e)->v == 12.0f);  // Copy then +2
    w.Shutdown();
}

// --- Undeclared system acts as a serial barrier | 序列屏障 ---
void TestBarrier() {
    ECS::World w;
    w.Initialize();
    w.GetSystemScheduler().RegisterSystem(std::make_unique<WriteA>());
    auto* legacy = new LegacyBarrier();
    w.GetSystemScheduler().RegisterSystem(std::unique_ptr<ECS::System>(legacy));
    w.GetSystemScheduler().RegisterSystem(std::make_unique<WriteB>());

    auto st = w.GetSystemScheduler().UpdateSystemsWaves(&w, 0.016f);
    CHECK(st.waves == 3);          // A | barrier | B
    CHECK(legacy->ran == 1);
    w.Shutdown();
}

// --- Parallel == sequential results | 平行與循序結果一致 ---
void TestSequentialEquivalence() {
    float seq[2] = {0, 0}, par[2] = {0, 0};
    for (int mode = 0; mode < 2; ++mode) {
        ECS::World w;
        w.Initialize();
        w.RegisterComponent<CompA>("CompA");
        w.RegisterComponent<CompB>("CompB");
        for (int i = 0; i < 50; ++i) {
            auto e = w.CreateEntity();
            w.AddComponent(e, CompA{});
            w.AddComponent(e, CompB{});
        }
        w.GetSystemScheduler().RegisterSystem(std::make_unique<WriteA>());
        w.GetSystemScheduler().RegisterSystem(std::make_unique<WriteB>());
        w.GetSystemScheduler().RegisterSystem(std::make_unique<CopyAToB>());

        if (mode == 0) {
            w.Update(0.016f);
        } else {
            GPU::CPUBackend be(4);
            w.UpdateParallel(0.016f, BackendExecutor(be));
        }
        float sa = 0, sb = 0;
        w.ForEach<CompA, CompB>(
            [&](ECS::Entity, CompA& a, CompB& b) { sa += a.v; sb += b.v; });
        seq[mode] = sa;
        par[mode] = sb;
        w.Shutdown();
    }
    CHECK(seq[0] == seq[1]);
    CHECK(par[0] == par[1]);
}

// --- Real parallelism: two systems rendezvous mid-wave ---
// 真實平行：兩系統在波中會合
void TestRealParallelism() {
    ECS::World w;
    w.Initialize();

    // Each system spins until BOTH have arrived — only possible if they
    // truly run concurrently (timeout guards the serial case).
    // 兩系統各自自旋等待對方到達 — 唯有真並行才能通過。
    std::atomic<int> arrived{0};
    std::atomic<bool> both{false};
    auto rendezvous = [&] {
        arrived.fetch_add(1);
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (arrived.load() < 2 &&
               std::chrono::steady_clock::now() < deadline) {
        }
        if (arrived.load() == 2) both = true;
    };
    struct Sys : ECS::System {
        std::function<void()> fn;
        void Update(ECS::World*, float) override { fn(); }
        void DescribeAccess(ECS::AccessSpec& a) const override {
            a.Read<CompA>();  // read-only → independent
        }
    };
    auto s1 = std::make_unique<Sys>(); s1->fn = rendezvous;
    auto s2 = std::make_unique<Sys>(); s2->fn = rendezvous;
    w.GetSystemScheduler().RegisterSystem(std::move(s1));
    w.GetSystemScheduler().RegisterSystem(std::move(s2));

    GPU::CPUBackend be(2);
    auto st = w.UpdateParallel(0.016f, BackendExecutor(be));
    CHECK(st.waves == 1);
    CHECK(both.load());            // actually ran concurrently
    w.Shutdown();
}

int main() {
    std::printf("=== StonePotatoes Parallel Scheduler Tests ===\n");

    TestWaveFormation();
    TestOrdering();
    TestBarrier();
    TestSequentialEquivalence();
    TestRealParallelism();

    std::printf("=== %d passed, %d failed ===\n", g_Passed, g_Failed);
    return g_Failed == 0 ? 0 : 1;
}
