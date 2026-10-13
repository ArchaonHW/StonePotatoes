// StonePotatoes Phase 1 unit/integration tests — self-contained runner,
// no external test framework dependency.
// StonePotatoes 第一階段單元/整合測試 — 自含執行器，不依賴外部測試框架。

#include "core/memory/MemoryPool.h"
#include "core/memory/StackAllocator.h"
#include "core/memory/ArenaAllocator.h"
#include "core/io/FileSystem.h"
#include "core/config/Config.h"
#include "ecs/World.h"
#include "ecs/events/EventBus.h"
#include "math/RiemannZeta.h"
#include "math/ZetaSpectral.h"
#include "iqgpu/ZetaCompute.h"
#include "iqgpu/IQGPURuntime.h"
#include "gpu/CPUBackend.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace StonePotatoes;

// --- Minimal test harness | 最小測試框架 ---
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

struct Pod { float x; float y; float z; int tag; };

// --- EntityManager | 實體管理器 ---
void TestEntityManager() {
    ECS::EntityManager mgr;

    ECS::Entity e0 = mgr.CreateEntity();
    CHECK(e0.IsValid());
    CHECK(mgr.IsEntityValid(e0));
    CHECK(mgr.GetEntityCount() == 1);

    ECS::Entity e1 = mgr.CreateEntity();
    CHECK(e1 != e0);
    CHECK(mgr.GetEntityCount() == 2);

    mgr.DestroyEntity(e0);
    CHECK(!mgr.IsEntityValid(e0));           // stale handle invalidated
    CHECK(mgr.IsEntityValid(e1));
    CHECK(mgr.GetEntityCount() == 1);

    ECS::Entity e2 = mgr.CreateEntity();      // reuses e0's index, new gen
    CHECK(e2.GetIndex() == e0.GetIndex());
    CHECK(e2.GetGeneration() != e0.GetGeneration());
    CHECK(mgr.IsEntityValid(e2));
}

// --- MemoryPool | 記憶體池 ---
void TestMemoryPool() {
    Memory::MemoryPool pool(sizeof(Pod), 4);

    void* a = pool.Allocate();
    void* b = pool.Allocate();
    CHECK(a && b && a != b);
    CHECK(pool.GetUsedBlocks() == 2);
    CHECK(pool.Owns(a));

    pool.Free(a);
    CHECK(pool.GetUsedBlocks() == 1);

    // Exhaust the pool
    void* c = pool.Allocate();
    void* d = pool.Allocate();
    void* e = pool.Allocate();
    void* overflow = pool.Allocate();          // 5th alloc on a 4-block pool
    CHECK(c && d && e);
    CHECK(overflow == nullptr);
    CHECK(pool.GetFreeBlocks() == 0);

    pool.Reset();
    CHECK(pool.GetUsedBlocks() == 0);
    CHECK(pool.GetFreeBlocks() == 4);
}

// --- StackAllocator | 堆疊分配器 ---
void TestStackAllocator() {
    Memory::StackAllocator stack(1024);

    void* p1 = stack.Allocate(256);
    CHECK(p1 != nullptr);
    CHECK(stack.GetUsed() >= 256);

    auto marker = stack.GetMarker();
    void* p2 = stack.Allocate(256);
    CHECK(p2 != nullptr);
    stack.FreeToMarker(marker);
    CHECK(stack.GetUsed() < 1024);

    // After the rollback, p1 is again the last allocation — in-order LIFO free.
    stack.FreeLast(p1, 256);
    CHECK(stack.GetUsed() == 0);

    stack.Reset();
    CHECK(stack.GetUsed() == 0);

    void* tooBig = stack.Allocate(4096);
    CHECK(tooBig == nullptr);
}

// --- ArenaAllocator | 區域分配器 ---
void TestArenaAllocator() {
    Memory::ArenaAllocator arena(512);

    void* small = arena.Allocate(256);
    CHECK(small != nullptr);
    CHECK(arena.GetChunkCount() == 1);

    void* big = arena.Allocate(4096);          // forces a new chunk
    CHECK(big != nullptr);
    CHECK(arena.GetChunkCount() == 2);
    CHECK(arena.GetTotalAllocated() >= 4096);

    arena.Reset();
    CHECK(arena.GetTotalUsed() == 0);
    CHECK(arena.GetChunkCount() == 2);         // chunks retained for reuse
}

// --- FileSystem | 檔案系統 ---
void TestFileSystem() {
    const char* path = "test_io_tmp.txt";
    const char* contents = "hello StonePotatoes";

    CHECK(IO::FileSystem::WriteTextFile(path, contents));
    CHECK(IO::FileSystem::Exists(path));
    CHECK(IO::FileSystem::IsFile(path));
    CHECK(IO::FileSystem::GetFileSize(path) == (int64_t)std::strlen(contents));

    std::string read;
    CHECK(IO::FileSystem::ReadTextFile(path, read));
    CHECK(read == contents);

    CHECK(IO::FileSystem::GetExtension("foo/bar.txt") == ".txt");
    CHECK(IO::FileSystem::GetStem("foo/bar.txt") == "bar");
    CHECK(IO::FileSystem::GetFilename("foo/bar.txt") == "bar.txt");

    CHECK(IO::FileSystem::Delete(path));
    CHECK(!IO::FileSystem::Exists(path));
}

// --- Config JSON | 設定 JSON ---
void TestConfig() {
    const char* path = "test_config_tmp.json";
    const char* json = R"({
        "window": { "width": 1920, "height": 1080, "title": "Test" },
        "debug": true,
        "scale": 2.5,
        "tags": ["a", "b", "c"]
    })";
    CHECK(IO::FileSystem::WriteTextFile(path, json));

    auto& cfg = Config::ConfigManager::GetInstance();
    cfg.Clear();
    CHECK(cfg.LoadFromFile(path));

    CHECK(cfg.GetInt("window.width", 0) == 1920);
    CHECK(cfg.GetInt("window.height", 0) == 1080);
    CHECK(std::strcmp(cfg.GetString("window.title", ""), "Test") == 0);
    CHECK(cfg.GetBool("debug", false) == true);
    CHECK(cfg.GetDouble("scale", 0.0) == 2.5);
    CHECK(std::strcmp(cfg.GetString("tags.2", ""), "c") == 0);
    CHECK(cfg.GetInt("missing", 7) == 7);

    // Save round-trip
    const char* out = "test_config_out.json";
    cfg.SetString("added", "value");
    CHECK(cfg.SaveToFile(out));
    cfg.Clear();
    CHECK(cfg.LoadFromFile(out));
    CHECK(std::strcmp(cfg.GetString("added", ""), "value") == 0);
    CHECK(cfg.GetInt("window.width", 0) == 1920);

    IO::FileSystem::Delete(path);
    IO::FileSystem::Delete(out);
}

// --- ECS: components, archetypes, query | ECS：組件、archetype、查詢 ---
void TestECS() {
    ECS::World world;
    world.Initialize();
    world.RegisterComponent<Pod>("Pod");

    ECS::Entity e = world.CreateEntity();
    CHECK(e.IsValid());
    CHECK(world.IsEntityValid(e));

    world.AddComponent<Pod>(e, Pod{1.0f, 2.0f, 3.0f, 42});
    CHECK(world.HasComponent<Pod>(e));

    Pod* p = world.GetComponent<Pod>(e);
    CHECK(p != nullptr);
    CHECK(p->tag == 42 && p->x == 1.0f);

    // Second entity, same archetype
    ECS::Entity e2 = world.CreateEntity();
    world.AddComponent<Pod>(e2, Pod{0, 0, 0, 7});

    int seen = 0;
    int tagSum = 0;
    world.ForEach<Pod>([&](ECS::Entity, Pod& pod) {
        ++seen;
        tagSum += pod.tag;
    });
    CHECK(seen == 2);
    CHECK(tagSum == 49);

    // Query with With/Without
    struct Marker { int m; };
    world.RegisterComponent<Marker>("Marker");
    world.AddComponent<Marker>(e2, Marker{1});
    auto q = world.MakeQuery<Pod>();
    q.Except<Marker>();
    CHECK(q.Count(world.GetArchetypeManager()) == 1);

    // Component removal migrates the entity
    world.RemoveComponent<Marker>(e2);
    CHECK(!world.HasComponent<Marker>(e2));
    CHECK(world.HasComponent<Pod>(e2));

    // Destroy cleans storage
    world.DestroyEntity(e);
    CHECK(!world.IsEntityValid(e));
    CHECK(!world.HasComponent<Pod>(e));

    world.Shutdown();
}

// --- EventBus | 事件匯流排 ---
struct TestEvent { int value; };

void TestEventBus() {
    ECS::EventBus bus;

    int immediate = 0;
    int deferred = 0;
    bus.Subscribe<TestEvent>([&](const TestEvent& e) { immediate += e.value; });
    bus.Publish(TestEvent{5});
    CHECK(immediate == 5);

    bus.Subscribe<TestEvent>([&](const TestEvent& e) { deferred += e.value; });
    bus.Queue(TestEvent{3});
    CHECK(deferred == 0);                      // not yet dispatched
    bus.Dispatch();
    CHECK(deferred == 3);
    CHECK(immediate == 8);                     // queued hits all subscribers
}

// --- World save/load | 世界存檔/讀檔 ---
void TestWorldSaveLoad() {
    const char* path = "test_world_tmp.bin";

    {
        ECS::World world;
        world.Initialize();
        world.RegisterComponent<Pod>("Pod");
        for (int i = 0; i < 50; ++i) {
            ECS::Entity e = world.CreateEntity();
            world.AddComponent<Pod>(
                e, Pod{float(i), float(i * 2), 0.0f, i});
        }
        CHECK(world.SaveToFile(path));
        world.Shutdown();
    }

    {
        ECS::World world;
        world.Initialize();
        world.RegisterComponent<Pod>("Pod");
        CHECK(world.LoadFromFile(path));
        CHECK(world.GetEntityManager().GetEntityCount() == 50);

        // Verify serialized data survived the round-trip.
        int sum = 0;
        int count = 0;
        world.ForEach<Pod>([&](ECS::Entity, Pod& pod) {
            sum += pod.tag;
            ++count;
        });
        CHECK(count == 50);
        CHECK(sum == 49 * 50 / 2);
        world.Shutdown();
    }

    IO::FileSystem::Delete(path);
}

// --- Zeta math core + IQGPU parallel zero scan ---
// ζ 數學核心 + IQGPU 平行零點掃描
void TestZeta() {
    using namespace Math::Zeta;

    // Known first zeros (Odlyzko tables) | 已知首批零點
    CHECK(std::fabs(HardyZ(14.134725141734693)) < 0.01);
    CHECK(std::fabs(HardyZ(21.022039638771554)) < 0.01);

    // Gram point: θ(g_0) = 0 → SmoothZeroCount(g_0) = 1
    const double g0 = GramPoint(0);
    CHECK(std::fabs(SmoothZeroCount(g0) - 1.0) < 1e-6);

    // IQGPU parallel zero scan over [13,30): must find the three
    // known zeros (14.13, 21.02, 25.01) refined to step·1e-4.
    // IQGPU 平行零點掃描 [13,30)：必須找到三個已知零點。
    IQGPU::IQGPURuntime rt;
    rt.Initialize();
    IQGPU::ZetaCompute zc(rt);
    ZeroSpectrum spec = zc.FindZeros(13.0, 30.0, 0.02);
    CHECK(spec.Size() >= 3);
    if (spec.Size() >= 3) {
        CHECK(std::fabs(spec.Gammas()[0] - 14.1347251) < 0.005);
        CHECK(std::fabs(spec.Gammas()[1] - 21.0220396) < 0.005);
        CHECK(std::fabs(spec.Gammas()[2] - 25.0108576) < 0.005);
    }

    // Spectrum statistics: GUE repulsion means spacing KS distance to
    // GUE should be much smaller than to Poisson.
    // 頻譜統計：GUE 排斥使間距對 GUE 的 KS 距離應遠小於 Poisson。
    if (spec.Size() >= 3) {
        CHECK(spec.KSDistanceGUE() < spec.KSDistancePoisson());
    }
    rt.Shutdown();
}

int main() {
    std::printf("=== StonePotatoes Phase 1 Tests ===\n");

    TestEntityManager();
    TestMemoryPool();
    TestStackAllocator();
    TestArenaAllocator();
    TestFileSystem();
    TestConfig();
    TestECS();
    TestEventBus();
    TestWorldSaveLoad();
    TestZeta();

    std::printf("=== %d passed, %d failed ===\n", g_Passed, g_Failed);
    return g_Failed == 0 ? 0 : 1;
}
