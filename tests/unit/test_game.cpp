#include "game/Game.h"
#include "ecs/World.h"

#include <memory>
#include <vector>

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (false)

namespace {
struct RecordingScene final : StonePotatoes::Game::Scene {
    std::vector<int> calls;
    float lastDelta = 0.0f;

    void OnEnter(StonePotatoes::ECS::World&) override { calls.push_back(1); }
    void Update(StonePotatoes::ECS::World&, float deltaTime) override {
        calls.push_back(2);
        lastDelta = deltaTime;
    }
    void OnExit(StonePotatoes::ECS::World&) override { calls.push_back(3); }
};
}

int main() {
    StonePotatoes::ECS::World world;
    world.Initialize();

    StonePotatoes::Game::Game game;
    CHECK(!game.Start(world));

    auto scene = std::make_unique<RecordingScene>();
    auto* recorded = scene.get();
    CHECK(game.SetScene(std::move(scene)));
    CHECK(game.Start(world));
    CHECK(game.IsRunning());
    CHECK(!game.SetScene(std::make_unique<RecordingScene>()));

    game.Update(0.25f);
    CHECK(recorded->lastDelta == 0.25f);
    CHECK((recorded->calls == std::vector<int>{1, 2}));

    game.Stop();
    CHECK(!game.IsRunning());
    CHECK((recorded->calls == std::vector<int>{1, 2, 3}));
    game.Update(1.0f);
    CHECK((recorded->calls == std::vector<int>{1, 2, 3}));

    world.Shutdown();
    return 0;
}
