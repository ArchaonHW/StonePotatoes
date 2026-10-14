#include <ecs/World.h>
#include <data/AssetCache.h>
#include <game/Game.h>
#include <game/GameConfig.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

struct Position {
    float x = 0.0f;
};

struct Velocity {
    float x = 0.0f;
};

class MovementSystem final : public StonePotatoes::ECS::System {
public:
    void Update(StonePotatoes::ECS::World* world, float deltaTime) override {
        world->ForEach<Position, Velocity>(
            [deltaTime](StonePotatoes::ECS::Entity, Position& position,
                        Velocity& velocity) {
                position.x += velocity.x * deltaTime;
            });
    }
};

class ConsumerScene final : public StonePotatoes::Game::Scene {
public:
    void Update(StonePotatoes::ECS::World&, float deltaTime) override {
        observedDelta = deltaTime;
        ++updateCount;
    }
    float observedDelta = 0.0f;
    int updateCount = 0;
};

class TransitionScene final : public StonePotatoes::Game::Scene {
public:
    TransitionScene(int sceneId, std::vector<std::string>& events)
        : id(sceneId), lifecycleEvents(events) {}

    void OnEnter(StonePotatoes::ECS::World&) override {
        lifecycleEvents.push_back("enter" + std::to_string(id));
    }
    void Update(StonePotatoes::ECS::World&, float) override {
        lifecycleEvents.push_back("update" + std::to_string(id));
    }
    void OnExit(StonePotatoes::ECS::World&) override {
        lifecycleEvents.push_back("exit" + std::to_string(id));
    }

private:
    int id;
    std::vector<std::string>& lifecycleEvents;
};

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    const StonePotatoes::Game::GameConfig defaults;
    if (defaults.windowWidth != 1280 || defaults.windowHeight != 720) return 9;

    StonePotatoes::ECS::Entity savedEntity;
    {
        StonePotatoes::ECS::World world;
        world.RegisterComponent<Position>("Position");
        world.RegisterComponent<Velocity>("Velocity");
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<MovementSystem>());
        world.Initialize();

        savedEntity = world.CreateEntity();
        world.AddComponent(savedEntity, Position{1.0f});
        world.AddComponent(savedEntity, Velocity{2.0f});
        StonePotatoes::Game::Game game;
        auto scene = std::make_unique<ConsumerScene>();
        auto* sceneView = scene.get();
        if (!game.SetScene(std::move(scene)) || !game.Start(world)) return 7;
        game.Update(0.5f);
        if (std::fabs(sceneView->observedDelta - 0.5f) > 0.0001f) return 8;
        game.Stop();

        const auto* position = world.GetComponent<Position>(savedEntity);
        if (!position || std::fabs(position->x - 2.0f) > 0.0001f) return 2;
        if (!world.SaveToFile(argv[1])) return 3;
    }

    // Validate fixed-step Game APIs and their ECS behavior from the installed
    // package, independently of the repository's in-tree unit target.
    {
        StonePotatoes::ECS::World world;
        world.RegisterComponent<Position>("Position");
        world.RegisterComponent<Velocity>("Velocity");
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<MovementSystem>());
        world.Initialize();
        const auto entity = world.CreateEntity();
        world.AddComponent(entity, Position{});
        world.AddComponent(entity, Velocity{2.0f});

        StonePotatoes::Game::Game game;
        auto scene = std::make_unique<ConsumerScene>();
        auto* sceneView = scene.get();
        if (!game.SetScene(std::move(scene)) ||
            !game.SetFixedStep(0.1f, 2) || !game.Start(world)) {
            return 11;
        }
        game.Update(0.05f);
        if (sceneView->updateCount != 0) return 12;
        game.Update(0.05f);
        if (sceneView->updateCount != 1 ||
            std::fabs(sceneView->observedDelta - 0.1f) > 0.0001f) {
            return 13;
        }
        const auto* fixedPosition = world.GetComponent<Position>(entity);
        if (!fixedPosition || std::fabs(fixedPosition->x - 0.2f) > 0.0001f) {
            return 14;
        }
        game.Stop();
        if (!game.DisableFixedStep()) return 15;
        world.Shutdown();
    }

    // Verify that an installed consumer sees scene transitions at the update
    // boundary with the old scene exited before the new scene enters/updates.
    {
        StonePotatoes::ECS::World world;
        world.RegisterComponent<Position>("Position");
        world.RegisterComponent<Velocity>("Velocity");
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<MovementSystem>());
        world.Initialize();
        const auto entity = world.CreateEntity();
        world.AddComponent(entity, Position{});
        world.AddComponent(entity, Velocity{2.0f});

        std::vector<std::string> events;
        StonePotatoes::Game::Game game;
        if (!game.SetScene(std::make_unique<TransitionScene>(1, events)) ||
            !game.Start(world)) {
            return 16;
        }
        if (events.size() != 1 || events[0] != "enter1") return 17;
        if (!game.ChangeScene(std::make_unique<TransitionScene>(2, events))) {
            return 18;
        }
        if (events.size() != 1) return 19;

        game.Update(0.25f);
        const std::vector<std::string> expected{
            "enter1", "exit1", "enter2", "update2"};
        if (events != expected) return 20;
        const auto* transitionedPosition = world.GetComponent<Position>(entity);
        if (!transitionedPosition ||
            std::fabs(transitionedPosition->x - 0.5f) > 0.0001f) {
            return 21;
        }

        game.Stop();
        if (events.size() != 5 || events.back() != "exit2") return 22;
        world.Shutdown();
    }

    StonePotatoes::ECS::World restored;
    restored.RegisterComponent<Position>("Position");
    restored.RegisterComponent<Velocity>("Velocity");
    if (!restored.LoadFromFile(argv[1])) return 4;

    const auto* position = restored.GetComponent<Position>(savedEntity);
    const auto* velocity = restored.GetComponent<Velocity>(savedEntity);
    if (!position || !velocity) return 5;
    if (std::fabs(position->x - 2.0f) > 0.0001f ||
        std::fabs(velocity->x - 2.0f) > 0.0001f) {
        return 6;
    }

    const std::filesystem::path savePath(argv[1]);
    StonePotatoes::Data::AssetCache assets(savePath.parent_path().string());
    const auto saveBytes = assets.LoadBinary(savePath.filename().string());
    const auto sameSaveBytes = assets.LoadBinary(savePath.filename().string());
    if (!saveBytes || saveBytes->size() < 12 || saveBytes != sameSaveBytes) {
        return 10;
    }
    if (assets.CachedAssetCount() != 1) return 23;

    const auto textPath = savePath.parent_path() / "consumer_text.txt";
    {
        std::ofstream textFile(textPath, std::ios::binary);
        textFile << "first text";
        if (!textFile) return 24;
    }
    const auto text = assets.LoadText("consumer_text.txt");
    const auto sameText = assets.LoadText("consumer_text.txt");
    if (!text || *text != "first text" || sameText != text ||
        assets.CachedAssetCount() != 2) {
        return 25;
    }
    {
        std::ofstream textFile(textPath, std::ios::binary | std::ios::trunc);
        textFile << "updated text";
        if (!textFile) return 26;
    }
    const auto cachedText = assets.LoadText("consumer_text.txt");
    if (cachedText != text || !cachedText || *cachedText != "first text") {
        return 27;
    }
    if (!assets.Invalidate("consumer_text.txt") ||
        assets.CachedAssetCount() != 1) {
        return 28;
    }
    const auto updatedText = assets.LoadText("consumer_text.txt");
    if (!updatedText || *updatedText != "updated text" ||
        *text != "first text" || updatedText == text) {
        return 29;
    }

    if (assets.LoadText("../CMakeCache.txt") ||
        assets.LoadBinary(textPath.string())) {
        return 30;
    }
    assets.Clear();
    if (assets.CachedAssetCount() != 0) return 31;
    return 0;
}
