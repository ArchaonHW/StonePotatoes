#include "game/Game.h"
#include "game/GameConfig.h"
#include "ecs/World.h"

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
}

int main() {
    StonePotatoes::ECS::World world;
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

    world.Shutdown();
    return 0;
}
