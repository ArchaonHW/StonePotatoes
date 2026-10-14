#include <ecs/World.h>
#include <data/AssetCache.h>
#include <core/plugin/Plugin.h>
#include <game/Game.h>
#include <game/GameConfig.h>
#include <api/cpp/StonePotatoes.h>

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

class ConsumerPlugin final : public StonePotatoes::Plugin::Plugin {
public:
    ConsumerPlugin(std::string name, std::vector<std::string> dependencies,
                   std::vector<std::string>& events)
        : Plugin(std::move(name), "1.0.0", std::move(dependencies)),
          lifecycleEvents(events) {}

protected:
    bool OnLoad() override {
        lifecycleEvents.push_back(std::string("load:") + GetName());
        return true;
    }
    void OnUnload() override {
        lifecycleEvents.push_back(std::string("unload:") + GetName());
    }

private:
    std::vector<std::string>& lifecycleEvents;
};

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    const StonePotatoes::Game::GameConfig defaults;
    if (defaults.windowWidth != 1280 || defaults.windowHeight != 720) return 9;

    // Exercise only the installed public C++ facade: registration, scene
    // lifecycle and frame updates through StonePotatoes::Runtime.
    {
        StonePotatoes::API::Runtime runtime;
        runtime.GetWorld().RegisterComponent<Position>("RuntimePosition");
        auto scene = std::make_unique<ConsumerScene>();
        auto* sceneView = scene.get();
        if (!runtime.SetScene(std::move(scene)) || !runtime.Start() ||
            !runtime.IsRunning()) {
            return 41;
        }
        runtime.Update(0.25f);
        if (sceneView->updateCount != 1 ||
            std::fabs(sceneView->observedDelta - 0.25f) > 0.0001f) {
            return 42;
        }
        runtime.Stop();
        if (runtime.IsRunning()) return 43;
    }

    // Exercise the installed Core target's public PluginManager API.
    {
        std::vector<std::string> events;
        StonePotatoes::Plugin::PluginManager plugins;
        if (!plugins.RegisterPlugin(std::unique_ptr<StonePotatoes::Plugin::Plugin>(
                new ConsumerPlugin("renderer", {"foundation"}, events))) ||
            !plugins.RegisterPlugin(std::unique_ptr<StonePotatoes::Plugin::Plugin>(
                new ConsumerPlugin("foundation", {}, events)))) {
            return 36;
        }
        if (!plugins.RegisterPlugin(std::unique_ptr<StonePotatoes::Plugin::Plugin>(
                new ConsumerPlugin("audio", {}, events)))) {
            return 37;
        }
        const std::vector<std::string> expectedOrder{
            "foundation", "renderer", "audio"};
        if (plugins.ResolveLoadOrder() != expectedOrder ||
            !plugins.LoadAllPlugins()) {
            return 38;
        }
        const std::vector<std::string> expectedLoads{
            "load:foundation", "load:renderer", "load:audio"};
        if (events != expectedLoads || !plugins.GetPlugin("renderer") ||
            !plugins.GetPlugin("renderer")->IsLoaded()) {
            return 39;
        }
        plugins.UnloadAllPlugins();
        const std::vector<std::string> expectedLifecycle{
            "load:foundation", "load:renderer", "load:audio",
            "unload:audio", "unload:renderer", "unload:foundation"};
        if (events != expectedLifecycle ||
            plugins.GetPlugin("foundation")->IsLoaded()) {
            return 40;
        }
    }

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

    // Malformed saves are validated before LoadFromFile mutates the world.
    auto writeBytes = [](const std::filesystem::path& path,
                         const std::vector<std::uint8_t>& bytes) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char*>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()));
        return static_cast<bool>(file);
    };
    const auto invalidSavePath = savePath.parent_path() / "invalid_save.bin";
    const std::vector<std::uint8_t> unsupportedVersion{
        0x44, 0x57, 0x50, 0x53, // SPWD, little endian
        0x63, 0x00, 0x00, 0x00, // unsupported format version 99
        0x00, 0x00, 0x00, 0x00  // entity count
    };
    if (!writeBytes(invalidSavePath, unsupportedVersion) ||
        restored.LoadFromFile(invalidSavePath.string().c_str())) {
        return 32;
    }
    auto truncatedSave = *saveBytes;
    truncatedSave.resize(8);
    if (!writeBytes(invalidSavePath, truncatedSave) ||
        restored.LoadFromFile(invalidSavePath.string().c_str())) {
        return 33;
    }
    auto trailingSave = *saveBytes;
    trailingSave.push_back(0xFF);
    if (!writeBytes(invalidSavePath, trailingSave) ||
        restored.LoadFromFile(invalidSavePath.string().c_str())) {
        return 34;
    }
    const auto* preservedPosition = restored.GetComponent<Position>(savedEntity);
    const auto* preservedVelocity = restored.GetComponent<Velocity>(savedEntity);
    if (restored.GetEntityManager().GetEntityCount() != 1 ||
        !restored.IsEntityValid(savedEntity) || !preservedPosition ||
        !preservedVelocity || std::fabs(preservedPosition->x - 2.0f) > 0.0001f ||
        std::fabs(preservedVelocity->x - 2.0f) > 0.0001f) {
        return 35;
    }
    return 0;
}
