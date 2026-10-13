#include "core/platform/Platform.h"
#include "core/platform/Window.h"
#include "core/time/Time.h"
#include "core/logging/Logger.h"
#include "core/config/Config.h"
#include "core/io/FileSystem.h"
#include "ecs/World.h"
#include "ecs/entity/Entity.h"
#include "ecs/system/System.h"
#include "self_developing_agent/MetaProgrammingAgent.h"

#include <iostream>
#include <memory>

// Simple movement system for demonstration
// 簡單移動系統用於演示
class MovementSystem : public StonePotatoes::ECS::System {
public:
    void Update(StonePotatoes::ECS::World* world, float deltaTime) override {
        (void)world; // Suppress unused parameter warning
        (void)deltaTime; // Suppress unused parameter warning
        // TODO: Implement actual movement system with transform and velocity components
        // TODO：實作實際移動系統與變換和速度組件
        // For now, this is a stub
        // 目前這是一個存根
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

    // Register movement system
    // 註冊移動系統
    auto movementSystem = std::make_unique<MovementSystem>();
    world.GetSystemScheduler().RegisterSystem(std::move(movementSystem));

    // Create 100 test entities
    // 建立 100 個測試實體
    STONEPOTATOES_LOG_INFO("Creating 100 test entities...");
    STONEPOTATOES_LOG_INFO("建立 100 個測試實體...");
    for (int i = 0; i < 100; i++) {
        StonePotatoes::ECS::Entity entity = world.GetEntityManager().CreateEntity();
        if (entity.IsValid()) {
            // TODO: Add components to entities
            // TODO：為實體添加組件
        }
    }
    STONEPOTATOES_LOG_INFO("Created %u entities", world.GetEntityManager().GetEntityCount());
    STONEPOTATOES_LOG_INFO("已建立 %u 個實體", world.GetEntityManager().GetEntityCount());

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
