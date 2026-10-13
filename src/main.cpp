#include "core/platform/Platform.h"
#include "core/platform/Window.h"
#include "core/time/Time.h"
#include "core/logging/Logger.h"
#include "ecs/World.h"
#include "ecs/entity/Entity.h"
#include "ecs/system/System.h"

#include <iostream>
#include <memory>

// Simple movement system for demonstration
class MovementSystem : public Genesis::ECS::System {
public:
    void Update(Genesis::ECS::World* world, float deltaTime) override {
        (void)world; // Suppress unused parameter warning
        (void)deltaTime; // Suppress unused parameter warning
        // TODO: Implement actual movement system with transform and velocity components
        // For now, this is a stub
    }
};

int main(int argc, char* argv[]) {
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning

    GENESIS_LOG_INFO("Genesis Engine - Phase 1 Core");

    // Initialize platform
    if (!Genesis::Platform::Initialize()) {
        GENESIS_LOG_FATAL("Failed to initialize platform");
        return -1;
    }

    // Initialize time manager
    Genesis::Time::TimeManager::GetInstance().Initialize();

    // Create window
    Genesis::Platform::WindowConfig windowConfig;
    windowConfig.width = 1280;
    windowConfig.height = 720;
    windowConfig.title = "Genesis Engine - Phase 1 Core";

    Genesis::Platform::Window window;
    if (!window.Create(windowConfig)) {
        GENESIS_LOG_FATAL("Failed to create window");
        Genesis::Platform::Shutdown();
        return -1;
    }

    GENESIS_LOG_INFO("Window created: %dx%d", window.GetWidth(), window.GetHeight());

    // Create ECS world
    Genesis::ECS::World world;
    world.Initialize();

    // Register movement system
    auto movementSystem = std::make_unique<MovementSystem>();
    world.GetSystemScheduler().RegisterSystem(std::move(movementSystem));

    // Create 100 test entities
    GENESIS_LOG_INFO("Creating 100 test entities...");
    for (int i = 0; i < 100; i++) {
        Genesis::ECS::Entity entity = world.GetEntityManager().CreateEntity();
        if (entity.IsValid()) {
            // TODO: Add components to entities
        }
    }
    GENESIS_LOG_INFO("Created %u entities", world.GetEntityManager().GetEntityCount());

    // Main loop
    GENESIS_LOG_INFO("Entering main loop...");
    double frameCount = 0;
    double startTime = Genesis::Time::TimeManager::GetInstance().GetTotalTime();
    // Run for 3 seconds for testing, then exit
    while (!window.ShouldClose() && (Genesis::Time::TimeManager::GetInstance().GetTotalTime() - startTime) < 3.0) {
        // Poll events
        window.PollEvents();

        // Update time
        Genesis::Time::TimeManager::GetInstance().Update();

        // Update world
        world.Update(static_cast<float>(Genesis::Time::TimeManager::GetInstance().GetDeltaTime()));

        // Swap buffers
        window.SwapBuffers();

        frameCount++;
    }

    GENESIS_LOG_INFO("Main loop ended. Total frames: %.0f", frameCount);

    // Shutdown
    world.Shutdown();
    window.Destroy();
    Genesis::Time::TimeManager::GetInstance().Shutdown();
    Genesis::Platform::Shutdown();

    GENESIS_LOG_INFO("Genesis Engine shutdown complete");
    return 0;
}
