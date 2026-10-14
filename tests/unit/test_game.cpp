#include "game/Game.h"
#include "game/GameConfig.h"
#include "ecs/World.h"
#include "ecs/system/System.h"

#include <limits>
#include <memory>
#include <string>
#include <vector>

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (false)

namespace {
struct RecordingScene final : StonePotatoes::Game::Scene {
    RecordingScene(int sceneId, std::vector<std::string>& calls)
        : id(sceneId), events(calls) {}

    void OnEnter(StonePotatoes::ECS::World&) override {
        events.push_back("enter" + std::to_string(id));
    }
    void Update(StonePotatoes::ECS::World&, float deltaTime) override {
        events.push_back("update" + std::to_string(id));
        lastDelta = deltaTime;
    }
    void OnExit(StonePotatoes::ECS::World&) override {
        events.push_back("exit" + std::to_string(id));
    }

    int id;
    std::vector<std::string>& events;
    float lastDelta = 0.0f;
};

struct DeltaRecorderSystem final : StonePotatoes::ECS::System {
    explicit DeltaRecorderSystem(std::vector<float>& deltas) : updates(deltas) {}
    void Update(StonePotatoes::ECS::World*, float deltaTime) override {
        updates.push_back(deltaTime);
    }
    std::vector<float>& updates;
};
}

int main() {
    std::vector<float> systemDeltas;
    auto recorder = std::make_unique<DeltaRecorderSystem>(systemDeltas);
    auto* recorderView = recorder.get();
    StonePotatoes::ECS::World world;
    world.GetSystemScheduler().RegisterSystem(std::move(recorder));
    world.Initialize();

    StonePotatoes::Game::GameConfig config;
    CHECK(config.windowWidth == 1280);
    CHECK(config.windowHeight == 720);
    CHECK(config.windowTitle == "StonePotatoes");

    StonePotatoes::Game::Game game;
    CHECK(!game.Start(world));
    std::vector<std::string> events;
    auto firstScene = std::make_unique<RecordingScene>(1, events);
    auto* firstSceneView = firstScene.get();
    CHECK(game.SetScene(std::move(firstScene)));
    CHECK(!game.ChangeScene(std::make_unique<RecordingScene>(2, events)));
    CHECK(game.Start(world));
    CHECK(game.IsRunning());
    CHECK(!game.SetScene(std::make_unique<RecordingScene>(3, events)));

    game.Update(0.25f);
    CHECK(firstSceneView->lastDelta == 0.25f);
    CHECK(events.size() == 2);
    CHECK(events[0] == "enter1" && events[1] == "update1");

    CHECK(game.ChangeScene(std::make_unique<RecordingScene>(2, events)));
    CHECK(events.size() == 2); // Changes are deferred to the next frame.
    game.Update(0.5f);
    CHECK(events.size() == 5);
    CHECK(events[2] == "exit1" && events[3] == "enter2" && events[4] == "update2");
    CHECK(!game.ChangeScene(nullptr));

    game.Stop();
    CHECK(!game.IsRunning());
    CHECK(events.size() == 6 && events[5] == "exit2");
    game.Update(1.0f);
    CHECK(events.size() == 6);

    // Fixed-step mode accumulates frame time and updates both the scene and
    // ECS once per simulation tick, while bounding catch-up work.
    systemDeltas.clear();

    StonePotatoes::Game::Game fixedGame;
    std::vector<std::string> fixedEvents;
    auto fixedScene = std::make_unique<RecordingScene>(4, fixedEvents);
    auto* fixedSceneView = fixedScene.get();
    CHECK(fixedGame.SetScene(std::move(fixedScene)));
    CHECK(!fixedGame.SetFixedStep(0.0f));
    CHECK(!fixedGame.SetFixedStep(std::numeric_limits<float>::infinity()));
    CHECK(!fixedGame.SetFixedStep(0.1f, 0));
    CHECK(fixedGame.SetFixedStep(0.1f, 2));
    CHECK(fixedGame.Start(world));
    CHECK(!fixedGame.SetFixedStep(0.2f));
    CHECK(!fixedGame.DisableFixedStep());

    fixedGame.Update(0.05f);
    CHECK(fixedEvents.size() == 1); // Only OnEnter; no simulation tick yet.
    CHECK(systemDeltas.empty());

    fixedGame.Update(0.05f);
    CHECK(fixedEvents.size() == 2 && fixedEvents[1] == "update4");
    CHECK(fixedSceneView->lastDelta == 0.1f);
    CHECK(systemDeltas.size() == 1 && systemDeltas[0] == 0.1f);

    fixedGame.Update(0.55f); // Runs two ticks and discards overdue whole ticks.
    CHECK(fixedEvents.size() == 4);
    CHECK(systemDeltas.size() == 3);
    CHECK(systemDeltas[1] == 0.1f && systemDeltas[2] == 0.1f);
    fixedGame.Update(0.05f); // Retained fractional remainder completes a tick.
    CHECK(fixedEvents.size() == 5);
    CHECK(systemDeltas.size() == 4);

    const auto eventCount = fixedEvents.size();
    fixedGame.Update(std::numeric_limits<float>::quiet_NaN());
    fixedGame.Update(-0.1f);
    CHECK(fixedEvents.size() == eventCount);

    fixedGame.Stop();
    CHECK(fixedGame.DisableFixedStep());
    CHECK(fixedGame.Start(world));
    fixedGame.Update(0.25f);
    CHECK(fixedSceneView->lastDelta == 0.25f);
    CHECK(systemDeltas.size() == 5 && systemDeltas.back() == 0.25f);
    fixedGame.Stop();
    CHECK(recorderView != nullptr);

    world.Shutdown();
    return 0;
}
