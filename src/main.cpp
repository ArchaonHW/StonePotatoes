#include "core/platform/Platform.h"
#include "core/platform/Window.h"
#include "core/time/Time.h"
#include "core/logging/Logger.h"
#include "core/config/Config.h"
#include "core/io/FileSystem.h"
#include "core/memory/MemoryPool.h"
#include "core/memory/StackAllocator.h"
#include "core/memory/ArenaAllocator.h"
#include "ecs/World.h"
#include "ecs/entity/Entity.h"
#include "ecs/system/System.h"
#include "self_developing_agent/MetaProgrammingAgent.h"
#include "iqgpu/IQGPURuntime.h"
#include "iqgpu/QuantumInspired.h"

#include <cmath>
#include <iostream>
#include <memory>

// --- Demo components (POD, serializable) | 演示組件（POD，可序列化） ---
struct Position { float x = 0, y = 0, z = 0; };
struct Velocity { float x = 0, y = 0, z = 0; };
struct Health   { int hp = 100; };

// Demo event | 演示事件
struct EntityDamagedEvent { StonePotatoes::ECS::Entity entity; int amount; };

// Real movement system: integrates Position from Velocity each tick.
// 真實移動系統：每 tick 以 Velocity 積分更新 Position。
class MovementSystem : public StonePotatoes::ECS::System {
public:
    void Update(StonePotatoes::ECS::World* world, float deltaTime) override {
        world->ForEach<Position, Velocity>(
            [deltaTime](StonePotatoes::ECS::Entity, Position& p, Velocity& v) {
                p.x += v.x * deltaTime;
                p.y += v.y * deltaTime;
                p.z += v.z * deltaTime;
            });
    }
};

int main(int argc, char* argv[]) {
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning

    // Enable file logging | 啟用檔案日誌
    StonePotatoes::Logging::Logger::GetInstance().EnableFileOutput(true, "stonepotatoes.log");

    STONEPOTATOES_LOG_INFO("StonePotatoes - Phase 1 Core with Self-Developing AI Agent");
    STONEPOTATOES_LOG_INFO("StonePotatoes - 第一階段核心配備自我開發 AI Agent");

    // Initialize platform
    // 初始化平台
    if (!StonePotatoes::Platform::Initialize()) {
        STONEPOTATOES_LOG_FATAL("Failed to initialize platform");
        STONEPOTATOES_LOG_FATAL("初始化平台失敗");
        return -1;
    }

    // Initialize time manager
    // 初始化時間管理器
    StonePotatoes::Time::TimeManager::GetInstance().Initialize();

    // Load configuration (JSON) | 載入設定（JSON）
    // Create a default config on first run | 首次執行時建立預設設定檔
    auto& config = StonePotatoes::Config::ConfigManager::GetInstance();
    if (!StonePotatoes::IO::FileSystem::Exists("engine_config.json")) {
        config.SetInt("window.width", 1280);
        config.SetInt("window.height", 720);
        config.SetString("window.title", "StonePotatoes - Self-Developing AI Agent");
        config.SetDouble("world.timeScale", 1.0);
        config.SetBool("debug.enabled", true);
        config.SaveToFile("engine_config.json");
    } else {
        config.LoadFromFile("engine_config.json");
    }
    STONEPOTATOES_LOG_INFO("Config: window=%dx%d title=%s",
        config.GetInt("window.width", 1280),
        config.GetInt("window.height", 720),
        config.GetString("window.title", "StonePotatoes"));

    // Create window
    // 建立視窗
    StonePotatoes::Platform::WindowConfig windowConfig;
    windowConfig.width = config.GetInt("window.width", 1280);
    windowConfig.height = config.GetInt("window.height", 720);
    windowConfig.title = config.GetString("window.title", "StonePotatoes");

    StonePotatoes::Platform::Window window;
    if (!window.Create(windowConfig)) {
        STONEPOTATOES_LOG_FATAL("Failed to create window");
        STONEPOTATOES_LOG_FATAL("建立視窗失敗");
        StonePotatoes::Platform::Shutdown();
        return -1;
    }

    STONEPOTATOES_LOG_INFO("Window created: %dx%d", window.GetWidth(), window.GetHeight());
    STONEPOTATOES_LOG_INFO("視窗已建立：%dx%d", window.GetWidth(), window.GetHeight());

    // Create ECS world
    // 建立 ECS 世界
    StonePotatoes::ECS::World world;
    world.Initialize();

    // Register component types with stable serialization names.
    // 以穩定的序列化名稱註冊組件型別。
    world.RegisterComponent<Position>("Position");
    world.RegisterComponent<Velocity>("Velocity");
    world.RegisterComponent<Health>("Health");

    // Register movement system
    // 註冊移動系統
    auto movementSystem = std::make_unique<MovementSystem>();
    world.GetSystemScheduler().RegisterSystem(std::move(movementSystem));

    // Subscribe to damage events (queued, dispatched at end of Update).
    // 訂閱傷害事件（佇列方式，於 Update 結尾分派）。
    world.GetEventBus().Subscribe<EntityDamagedEvent>(
        [](const EntityDamagedEvent& e) {
            STONEPOTATOES_LOG_INFO(
                "Event: entity %u took %d damage", e.entity.GetIndex(),
                e.amount);
        });

    // Create 100 test entities with real components.
    // 建立 100 個帶有真實組件的測試實體。
    STONEPOTATOES_LOG_INFO("Creating 100 test entities...");
    STONEPOTATOES_LOG_INFO("建立 100 個測試實體...");
    StonePotatoes::ECS::Entity firstEntity;
    for (int i = 0; i < 100; i++) {
        StonePotatoes::ECS::Entity entity = world.CreateEntity();
        if (i == 0) firstEntity = entity;
        world.AddComponent<Position>(entity,
                                     Position{static_cast<float>(i), 0.0f, 0.0f});
        world.AddComponent<Velocity>(entity, Velocity{1.0f, 0.5f, 0.0f});
        world.AddComponent<Health>(entity, Health{100});
    }
    STONEPOTATOES_LOG_INFO("Created %u entities", world.GetEntityManager().GetEntityCount());
    STONEPOTATOES_LOG_INFO("已建立 %u 個實體", world.GetEntityManager().GetEntityCount());
    STONEPOTATOES_LOG_INFO("Archetypes: %zu",
                           world.GetArchetypeManager().GetArchetypeCount());

    // Query demo: count entities with Position+Velocity (all 100 expected).
    // 查詢演示：計算擁有 Position+Velocity 的實體（預期全部 100 個）。
    auto moving = world.MakeQuery<Position, Velocity>();
    STONEPOTATOES_LOG_INFO("Query<Position,Velocity>: %zu entities",
                           moving.Count(world.GetArchetypeManager()));

    // Allocator smoke test | 分配器煙霧測試
    {
        StonePotatoes::Memory::MemoryPool pool(sizeof(Position), 8);
        void* a = pool.Allocate();
        void* b = pool.Allocate();
        pool.Free(a);
        STONEPOTATOES_LOG_INFO("MemoryPool: used=%zu free=%zu ownsA=%d",
            pool.GetUsedBlocks(), pool.GetFreeBlocks(), pool.Owns(b));

        StonePotatoes::Memory::StackAllocator stack(4096);
        auto marker = stack.GetMarker();
        void* s1 = stack.Allocate(256);
        void* s2 = stack.Allocate(128);
        stack.FreeToMarker(marker);
        STONEPOTATOES_LOG_INFO("StackAllocator: after rollback used=%zu (s1=%p s2=%p)",
            stack.GetUsed(), s1, s2);

        StonePotatoes::Memory::ArenaAllocator arena(1024);
        void* big = arena.Allocate(4096); // forces second chunk
        STONEPOTATOES_LOG_INFO("ArenaAllocator: chunks=%zu allocated=%zu (big=%p)",
            arena.GetChunkCount(), arena.GetTotalAllocated(), big);
    }

    // Resource system demo | 資源系統演示
    struct WorldClock { double elapsed = 0.0; };
    world.GetResourceManager().Insert<WorldClock>();
    world.GetResourceManager().Get<WorldClock>()->elapsed = 42.0;
    STONEPOTATOES_LOG_INFO("Resource WorldClock.elapsed = %.1f",
        world.GetResourceManager().Get<WorldClock>()->elapsed);

    // Queue a demo event (dispatched on first World::Update).
    // 佇列一個演示事件（於第一次 World::Update 時分派）。
    world.GetEventBus().Queue(EntityDamagedEvent{firstEntity, 25});

    // Save/Load round-trip demo | 儲存/載入往返演示
    world.SaveToFile("world_save.bin");
    {
        StonePotatoes::ECS::World loaded;
        loaded.Initialize();
        loaded.RegisterComponent<Position>("Position");
        loaded.RegisterComponent<Velocity>("Velocity");
        loaded.RegisterComponent<Health>("Health");
        loaded.LoadFromFile("world_save.bin");
        STONEPOTATOES_LOG_INFO("Loaded world entities: %u (expected 100)",
                               loaded.GetEntityManager().GetEntityCount());
        loaded.Shutdown();
    }

    // Initialize Self-Developing AI Agent
    // 初始化自我開發 AI Agent
    STONEPOTATOES_LOG_INFO("Initializing Self-Developing AI Agent...");
    STONEPOTATOES_LOG_INFO("初始化自我開發 AI Agent...");
    
    StonePotatoes::SelfDevelopingAgent::MetaProgrammingAgent agent;
    agent.Initialize();
    
    // Set development goal
    // 設定開發目標
    agent.SetDevelopmentGoal("Optimize ECS performance for 10,000 entities");
    agent.SetDevelopmentGoal("優化 10,000 個實體的 ECS 效能");

    // Run one development cycle
    // 執行一個開發週期
    STONEPOTATOES_LOG_INFO("Running development cycle...");
    STONEPOTATOES_LOG_INFO("執行開發週期...");
    agent.DevelopmentCycle();

    // Get agent statistics
    // 獲取 Agent 統計資料
    auto stats = agent.GetStatistics();
    STONEPOTATOES_LOG_INFO("Agent Statistics:");
    STONEPOTATOES_LOG_INFO("Code Generated: %d lines", stats.codeGeneratedLines);
    STONEPOTATOES_LOG_INFO("Tests Generated: %d", stats.testsGenerated);
    STONEPOTATOES_LOG_INFO("Bugs Found: %d", stats.bugsFound);
    STONEPOTATOES_LOG_INFO("Optimizations: %d", stats.optimizationsPerformed);
    STONEPOTATOES_LOG_INFO("Self-Improvement Score: %.2f", stats.selfImprovementScore);
    
    STONEPOTATOES_LOG_INFO("Agent 統計資料：");
    STONEPOTATOES_LOG_INFO("程式碼生成：%d 行", stats.codeGeneratedLines);
    STONEPOTATOES_LOG_INFO("測試生成：%d", stats.testsGenerated);
    STONEPOTATOES_LOG_INFO("錯誤發現：%d", stats.bugsFound);
    STONEPOTATOES_LOG_INFO("優化執行：%d", stats.optimizationsPerformed);
    STONEPOTATOES_LOG_INFO("自我改進分數：%.2f", stats.selfImprovementScore);

    // ------------------------------------------------------------------
    // IQGPU demonstration | IQGPU 演示
    // CPU reference backend — real parallel dispatch, honest metrics.
    // CPU 參考後端 — 真實平行派發、誠實的量測數據。
    // ------------------------------------------------------------------
    STONEPOTATOES_LOG_INFO("--- IQGPU Compute Demo | IQGPU 計算演示 ---");
    {
        StonePotatoes::IQGPU::IQGPURuntime iqgpu;
        iqgpu.Initialize();

        // 1) Parallel kernel: saxpy over 1M floats | 平行核心：一百萬筆 float 的 saxpy
        constexpr uint32_t N = 1'000'000;
        auto* bufX = iqgpu.CreateTypedBuffer<float>(N);
        auto* bufY = iqgpu.CreateTypedBuffer<float>(N);
        for (uint32_t i = 0; i < N; ++i) { bufX->Data()[i] = float(i); bufY->Data()[i] = 1.0f; }

        struct SaxpyData { float* x; float* y; float a; };
        SaxpyData sd{bufX->Data(), bufY->Data(), 2.5f};
        StonePotatoes::GPU::Kernel saxpy("saxpy",
            [](uint32_t gid, void* ud) {
                auto* d = static_cast<SaxpyData*>(ud);
                d->y[gid] = d->a * d->x[gid] + d->y[gid];
            });
        StonePotatoes::GPU::DispatchParams dp;
        dp.threadCount = N; dp.userData = &sd;
        auto ds = iqgpu.Dispatch(saxpy, dp);
        STONEPOTATOES_LOG_INFO(
            "[IQGPU] saxpy: %u elems, %u workers, %.2f ms, y[999999]=%.1f (expect 2499998.5)",
            ds.itemsProcessed, ds.threadsUsed, ds.dispatchMs,
            static_cast<double>(bufY->Data()[N - 1]));

        // 2) ComputeGraph: A -> B -> C ordering | 計算圖：A -> B -> C 順序
        auto& graph = iqgpu.GetGraph();
        std::vector<float> graphOut(8, 0.0f);
        auto nodeA = StonePotatoes::GPU::ComputeNode::MakeKernel(
            "A_generate",
            StonePotatoes::GPU::Kernel("gen", [](uint32_t gid, void* ud) {
                static_cast<float*>(ud)[gid] = float(gid * gid);
            }), 8, graphOut.data());
        auto nodeB = StonePotatoes::GPU::ComputeNode::MakeKernel(
            "B_scale",
            StonePotatoes::GPU::Kernel("scale", [](uint32_t gid, void* ud) {
                static_cast<float*>(ud)[gid] *= 2.0f;
            }), 8, graphOut.data());
        nodeB->AddDependency(nodeA);
        auto nodeC = StonePotatoes::GPU::ComputeNode::MakeTask(
            "C_verify", [&graphOut] {
                STONEPOTATOES_LOG_INFO(
                    "[IQGPU] graph result out[7]=%.0f (expect 98)",
                    static_cast<double>(graphOut[7]));
            });
        nodeC->AddDependency(nodeB);
        graph.AddNode(nodeA); graph.AddNode(nodeB); graph.AddNode(nodeC);
        auto gr = iqgpu.ExecuteGraph();
        STONEPOTATOES_LOG_INFO("[IQGPU] graph: %u nodes, %.2f ms, order=%s",
            gr.nodesExecuted, gr.totalMs,
            [&]{ std::string s; for (auto& n : gr.executionOrder) s += n + " "; return s; }().c_str());
        iqgpu.Shutdown();
    }

    // 3) Quantum-inspired algorithms (classical simulation)
    // 量子啟發演算法（傳統模擬）
    STONEPOTATOES_LOG_INFO("--- Quantum-Inspired Demo | 量子啟發演示 ---");
    {
        using namespace StonePotatoes::IQGPU;

        // Rastrigin-like multimodal energy: many local minima.
        // Rastrigin 式多峰能量函式：大量局部最小值。
        auto rastrigin = [](const std::vector<double>& x) {
            double e = 10.0 * double(x.size());
            for (double v : x) e += v * v - 10.0 * std::cos(2.0 * 3.14159265 * v);
            return e;
        };
        auto perturb = [](std::vector<double>& x, std::mt19937& rng) {
            std::uniform_int_distribution<size_t> pick(0, x.size() - 1);
            std::normal_distribution<double> n(0.0, 0.3);
            x[pick(rng)] += n(rng);
        };
        auto teleport = [](std::vector<double>& x, std::mt19937& rng) {
            std::uniform_real_distribution<double> u(-5.12, 5.12);
            for (auto& v : x) v = u(rng);
        };
        std::vector<double> init(8, 5.0);

        SimulatedAnnealer sa;
        auto rSA = sa.Optimize(init, rastrigin, perturb, 20000, 5.0, 0.999, 42);
        QuantumInspiredAnnealer qa;
        auto rQA = qa.Optimize(init, rastrigin, perturb, teleport,
                               20000, 5.0, 0.999, 0.08, 42);
        STONEPOTATOES_LOG_INFO(
            "[QI] Annealing 20k iters: classical E=%.2f (improved %.2f) | "
            "quantum-inspired E=%.2f (improved %.2f)",
            rSA.bestEnergy, rSA.improvement, rQA.bestEnergy, rQA.improvement);

        // Amplitude amplification: find multiples of 97 in [0, 1024).
        // 振幅放大：在 [0,1024) 中尋找 97 的倍數。
        AmplitudeAmplifier aa;
        auto rAA = aa.Search(1024, [](uint32_t i) { return i % 97 == 0 && i != 0; }, 8);
        double markedP = 0.0, unmarkedP = 0.0; uint32_t unmarkedN = 0;
        for (uint32_t i = 0; i < 1024; ++i) {
            bool marked = (i % 97 == 0 && i != 0);
            if (marked) markedP += rAA.probabilities[i];
            else { unmarkedP += rAA.probabilities[i]; ++unmarkedN; }
        }
        STONEPOTATOES_LOG_INFO(
            "[QI] AmplitudeAmp: %zu marked in 1024, %u rounds, "
            "markedP=%.1f%% vs uniform=%.1f%%",
            rAA.markedStates.size(), rAA.roundsApplied,
            markedP * 100.0,
            (double(rAA.markedStates.size()) / 1024.0) * 100.0);

        // Born-rule measurement | 玻恩定則測量
        ProbabilisticStateSpace pss(4, 7);
        pss.SetAmplitude(0, 1.0); pss.SetAmplitude(1, 2.0);
        pss.SetAmplitude(2, 0.5); pss.SetAmplitude(3, 1.5);
        pss.Normalize();
        uint32_t counts[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4000; ++i) ++counts[pss.Measure()];
        STONEPOTATOES_LOG_INFO(
            "[QI] Measure 4k samples: |0>=%.1f%% |1>=%.1f%% |2>=%.1f%% |3>=%.1f%%",
            counts[0] / 40.0, counts[1] / 40.0, counts[2] / 40.0, counts[3] / 40.0);
    }

    // Main loop
    // 主迴圈
    STONEPOTATOES_LOG_INFO("Entering main loop...");
    STONEPOTATOES_LOG_INFO("進入主迴圈...");
    double frameCount = 0;
    double startTime = StonePotatoes::Time::TimeManager::GetInstance().GetTotalTime();
    // Run for 3 seconds for testing, then exit
    // 執行 3 秒用於測試，然後退出
    while (!window.ShouldClose() && (StonePotatoes::Time::TimeManager::GetInstance().GetTotalTime() - startTime) < 3.0) {
        // Poll events
        // 輪詢事件
        window.PollEvents();

        // Update time
        // 更新時間
        StonePotatoes::Time::TimeManager::GetInstance().Update();

        // Update world
        // 更新世界
        world.Update(static_cast<float>(StonePotatoes::Time::TimeManager::GetInstance().GetDeltaTime()));

        // Swap buffers
        // 交換緩衝區
        window.SwapBuffers();

        frameCount++;
    }

    STONEPOTATOES_LOG_INFO("Main loop ended. Total frames: %.0f", frameCount);
    STONEPOTATOES_LOG_INFO("主迴圈結束。總幀數：%.0f", frameCount);

    // Shutdown
    // 關閉
    agent.Shutdown();
    world.Shutdown();
    window.Destroy();
    StonePotatoes::Time::TimeManager::GetInstance().Shutdown();
    StonePotatoes::Platform::Shutdown();

    STONEPOTATOES_LOG_INFO("StonePotatoes shutdown complete");
    STONEPOTATOES_LOG_INFO("StonePotatoes 關閉完成");
    return 0;
}
