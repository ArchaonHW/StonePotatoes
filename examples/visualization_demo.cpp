// StonePotatoes World Visualization Demo
// Shows NPCs (blue), food (green), threats (red), and memory circles.
// NPC（藍）、食物（綠）、威脅（紅）、記憶圈可視化。

#include "ai/NPC.h"
#include "core/platform/Platform.h"
#include "ecs/World.h"
#include "rendering/Renderer.h"
#include "world/procedural/ResourceScatter.h"
#include "world/procedural/WorldStreamer.h"
#include <imgui.h>

#include <chrono>
#include <cstdio>

using namespace StonePotatoes;
using namespace StonePotatoes::AI;

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

int main() {
    if (!Platform::Initialize()) {
        std::printf("[ERROR] Platform init failed\n");
        return 1;
    }

    // Setup world | 設定世界
    auto world = std::make_unique<ECS::World>();
    world->Initialize();
    RegisterNpcComponents(*world);
    auto& sched = world->GetSystemScheduler();
    sched.RegisterSystem(std::make_unique<PerceptionSystem>());
    sched.RegisterSystem(std::make_unique<DecisionSystem>());
    sched.RegisterSystem(std::make_unique<ActionSystem>());
    sched.RegisterSystem(std::make_unique<NpcMovementSystem>());
    sched.InitializeSystems(world.get());

    // Setup renderer | 設定渲染器
    Rendering::Renderer renderer;
    if (!renderer.Initialize({1280, 720, "StonePotatoes World"})) {
        std::printf("[ERROR] Renderer init failed\n");
        Platform::Shutdown();
        return 1;
    }

    // Populate | 填充實體
    for (int i = 0; i < 10; ++i) {
        const float x = (i % 5) * 20.0f - 40.0f;
        const float y = (i / 5) * 20.0f - 20.0f;
        auto npc = SpawnNpc(*world, x, y);
        world->GetComponent<Needs>(npc)->hunger = 0.8f;
    }
    for (int i = 0; i < 20; ++i) {
        SpawnFood(*world, (i % 5) * 15.0f - 30.0f,
                   (i / 5) * 15.0f - 15.0f);
    }
    SpawnThreat(*world, 15.0f, 0.0f);

    // Main loop | 主迴圈
    constexpr float dt = 0.016f;
    int frame = 0;
    float simTime = 0.0f;
    int eatenCount = 0;
    world->GetEventBus().Subscribe<FoodEatenEvent>(
        [&](const FoodEatenEvent&) { ++eatenCount; });

    while (!renderer.ShouldClose()) {
        renderer.BeginFrame();

        // Simulate | 模擬
        world->Update(dt);
        simTime += dt;
        ++frame;

        // Draw entities | 繪製實體
        world->ForEach<NPCPosition, Food>(
            [&](ECS::Entity, NPCPosition& p, Food&) {
                renderer.DrawPoint(p.x, p.y, 0.3f, {0.0f, 1.0f, 0.0f});
            });
        world->ForEach<NPCPosition, Threat>(
            [&](ECS::Entity, NPCPosition& p, Threat&) {
                renderer.DrawPoint(p.x, p.y, 0.5f, {1.0f, 0.0f, 0.0f});
            });
        world->ForEach<NPCPosition, Memory>(
            [&](ECS::Entity, NPCPosition& p, Memory& mem) {
                renderer.DrawPoint(p.x, p.y, 0.4f, {0.2f, 0.5f, 1.0f});
                for (uint32_t i = 0; i < mem.count; ++i) {
                    const auto& r = mem.records[i];
                    renderer.DrawCircle(r.x, r.y, 2.0f,
                                      {0.5f, 0.5f, 0.5f});
                }
            });

        // ImGui stats | ImGui 統計
        renderer.BeginImGui();
        ImGui::Text("Time: %.2f s", simTime);
        ImGui::Text("Frames: %d", frame);
        ImGui::Text("Eaten: %d", eatenCount);
        ImGui::Text("WASD: pan view | +/-: zoom");
        renderer.EndImGui();

        renderer.EndFrame();

        // Simple input handling | 簡單輸入處理
        // (Would use proper input system in full version)
        // （完整版本會用正式輸入系統）
    }

    world->Shutdown();
    renderer.Shutdown();
    Platform::Shutdown();
    std::printf("Visualization demo ended\n");
    return 0;
}
