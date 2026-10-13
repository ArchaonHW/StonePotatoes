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
