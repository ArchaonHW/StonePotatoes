#include <ecs/World.h>
#include <civilization/economic/SystemDynamics.h>
#include <civilization/economic/TradeNetwork.h>
#include <civilization/population/PopulationDynamics.h>
#include <networking/core/LockstepSync.h>
#include <data/AssetCache.h>
#include <core/plugin/Plugin.h>
#include <game/Game.h>
#include <game/GameConfig.h>
#include <physics/Physics2D.h>
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

    // Verify that the installed Physics target supplies its public headers,
    // transitive Math/ECS dependencies, and fixed-tick runtime integration.
    {
        StonePotatoes::ECS::World world;
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<StonePotatoes::Physics::Physics2DSystem>(
                StonePotatoes::Math::Vec2{0.0f, -8.0f}));
        world.Initialize();

        const auto entity = world.CreateEntity();
        world.AddComponent(entity,
                           StonePotatoes::Physics::Transform2D{{0.0f, 2.0f}});
        StonePotatoes::Physics::RigidBody2D body;
        body.inverseMass = 0.5f;
        if (!body.ApplyForce({4.0f, 0.0f})) return 44;
        world.AddComponent(entity, body);

        StonePotatoes::Game::Game game;
        if (!game.SetScene(std::make_unique<ConsumerScene>()) ||
            !game.SetFixedStep(0.125f) || !game.Start(world)) {
            return 45;
        }
        game.Update(0.0625f);
        const auto* beforeTick =
            world.GetComponent<StonePotatoes::Physics::Transform2D>(entity);
        if (!beforeTick || beforeTick->position.x != 0.0f ||
            beforeTick->position.y != 2.0f) {
            return 46;
        }

        game.Update(0.0625f);
        const auto* afterTick =
            world.GetComponent<StonePotatoes::Physics::Transform2D>(entity);
        const auto* afterTickBody =
            world.GetComponent<StonePotatoes::Physics::RigidBody2D>(entity);
        if (!afterTick || !afterTickBody ||
            std::fabs(afterTick->position.x - 0.03125f) > 0.0001f ||
            std::fabs(afterTick->position.y - 1.875f) > 0.0001f ||
            std::fabs(afterTickBody->velocity.x - 0.25f) > 0.0001f ||
            std::fabs(afterTickBody->velocity.y + 1.0f) > 0.0001f ||
            afterTickBody->accumulatedForce.x != 0.0f ||
            afterTickBody->accumulatedForce.y != 0.0f) {
            return 47;
        }

        game.Stop();
        world.Shutdown();
    }

    // Verify installed Physics collision behavior through its public headers:
    // swept AABB prevents a fast body from tunneling through a wall, and
    // Coulomb friction removes a low-speed tangential component at rest.
    {
        using namespace StonePotatoes::Physics;
        StonePotatoes::ECS::World world;
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<Physics2DSystem>(StonePotatoes::Math::Vec2{}));
        world.Initialize();

        const auto wall = world.CreateEntity();
        world.AddComponent(wall, Transform2D{{10.0f, 0.0f}});
        RigidBody2D wallBody;
        wallBody.inverseMass = 0.0f;
        world.AddComponent(wall, wallBody);
        world.AddComponent(wall, BoxCollider2D{{0.5f, 2.0f}});

        const auto fastBody = world.CreateEntity();
        world.AddComponent(fastBody, Transform2D{{0.0f, 0.0f}});
        RigidBody2D fastRigidBody;
        fastRigidBody.velocity = {120.0f, 0.0f};
        world.AddComponent(fastBody, fastRigidBody);
        world.AddComponent(fastBody, BoxCollider2D{{0.5f, 0.5f}});

        const auto floor = world.CreateEntity();
        world.AddComponent(floor, Transform2D{{30.0f, 0.0f}});
        RigidBody2D floorBody;
        floorBody.inverseMass = 0.0f;
        world.AddComponent(floor, floorBody);
        world.AddComponent(
            floor, BoxCollider2D{{2.0f, 0.5f}, 1.0f, 0.5f});

        const auto slider = world.CreateEntity();
        world.AddComponent(slider, Transform2D{{30.0f, 0.9f}});
        RigidBody2D sliderBody;
        sliderBody.velocity = {0.5f, -2.0f};
        world.AddComponent(slider, sliderBody);
        world.AddComponent(
            slider, BoxCollider2D{{0.5f, 0.5f}, 1.0f, 0.5f});

        world.Update(0.1f);

        const auto* stoppedTransform = world.GetComponent<Transform2D>(fastBody);
        const auto* stoppedBody = world.GetComponent<RigidBody2D>(fastBody);
        if (!stoppedTransform || !stoppedBody ||
            std::fabs(stoppedTransform->position.x - 9.0f) > 0.001f ||
            std::fabs(stoppedBody->velocity.x) > 0.0001f) {
            return 54;
        }

        const auto* restingTransform = world.GetComponent<Transform2D>(slider);
        const auto* restingBody = world.GetComponent<RigidBody2D>(slider);
        if (!restingTransform || !restingBody ||
            std::fabs(restingTransform->position.y - 1.0f) > 0.001f ||
            std::fabs(restingBody->velocity.x) > 0.0001f ||
            std::fabs(restingBody->velocity.y) > 0.0001f) {
            return 55;
        }

        world.Shutdown();
    }

    // Verify that the installed Physics target resolves a fast dynamic pair
    // before the boxes cross, using only the public API.
    {
        using namespace StonePotatoes::Physics;
        StonePotatoes::ECS::World world;
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<Physics2DSystem>(StonePotatoes::Math::Vec2{}));
        world.Initialize();

        const auto left = world.CreateEntity();
        world.AddComponent(left, Transform2D{{-5.0f, 0.0f}});
        RigidBody2D leftBody;
        leftBody.velocity = {100.0f, 0.0f};
        world.AddComponent(left, leftBody);
        world.AddComponent(left, BoxCollider2D{{0.5f, 0.5f}});

        const auto right = world.CreateEntity();
        world.AddComponent(right, Transform2D{{5.0f, 0.0f}});
        RigidBody2D rightBody;
        rightBody.velocity = {-100.0f, 0.0f};
        world.AddComponent(right, rightBody);
        world.AddComponent(right, BoxCollider2D{{0.5f, 0.5f}});

        world.Update(0.1f);
        const auto* leftTransform = world.GetComponent<Transform2D>(left);
        const auto* rightTransform = world.GetComponent<Transform2D>(right);
        const auto* stoppedLeft = world.GetComponent<RigidBody2D>(left);
        const auto* stoppedRight = world.GetComponent<RigidBody2D>(right);
        if (!leftTransform || !rightTransform || !stoppedLeft ||
            !stoppedRight ||
            std::fabs(leftTransform->position.x + 0.5f) > 0.001f ||
            std::fabs(rightTransform->position.x - 0.5f) > 0.001f ||
            std::fabs(stoppedLeft->velocity.x) > 0.0001f ||
            std::fabs(stoppedRight->velocity.x) > 0.0001f) {
            return 57;
        }
        world.Shutdown();
    }

    // Verify deterministic recovery when two installed-package bodies start
    // at the exact same position, where the contact has no geometric normal.
    {
        using namespace StonePotatoes::Physics;
        StonePotatoes::ECS::World world;
        auto physics = std::make_unique<Physics2DSystem>(
            StonePotatoes::Math::Vec2{});
        Physics2DSystem* physicsView = physics.get();
        world.GetSystemScheduler().RegisterSystem(std::move(physics));
        world.Initialize();

        const auto first = world.CreateEntity();
        world.AddComponent(first, Transform2D{{0.0f, 0.0f}});
        world.AddComponent(first, RigidBody2D{});
        world.AddComponent(first, BoxCollider2D{{0.5f, 0.5f}});

        const auto second = world.CreateEntity();
        world.AddComponent(second, Transform2D{{0.0f, 0.0f}});
        world.AddComponent(second, RigidBody2D{});
        world.AddComponent(second, BoxCollider2D{{0.5f, 0.5f}});

        world.Update(1.0f / 60.0f);
        const auto* firstTransform = world.GetComponent<Transform2D>(first);
        const auto* secondTransform = world.GetComponent<Transform2D>(second);
        if (!firstTransform || !secondTransform ||
            !(firstTransform->position.x < secondTransform->position.x) ||
            secondTransform->position.x - firstTransform->position.x <
                0.999f ||
            firstTransform->position.y != 0.0f ||
            secondTransform->position.y != 0.0f ||
            !physicsView->DidLastContactSolveConverge() ||
            physicsView->GetLastContactIterationCount() > 16) {
            return 58;
        }
        world.Shutdown();
    }

    // Verify the installed Physics target supports rotated box contacts.
    {
        using namespace StonePotatoes::Physics;
        constexpr float diagonal = 0.70710678118f;
        constexpr float rotation = 0.78539816339f;
        StonePotatoes::ECS::World world;
        auto physics = std::make_unique<Physics2DSystem>(
            StonePotatoes::Math::Vec2{});
        Physics2DSystem* physicsView = physics.get();
        world.GetSystemScheduler().RegisterSystem(std::move(physics));
        world.Initialize();

        const auto first = world.CreateEntity();
        world.AddComponent(first, Transform2D{{0.0f, 0.0f}});
        world.AddComponent(first, Rotation2D{rotation});
        world.AddComponent(first, RigidBody2D{});
        world.AddComponent(first, BoxCollider2D{{2.0f, 0.1f}});

        const auto second = world.CreateEntity();
        world.AddComponent(second,
                           Transform2D{{-0.15f * diagonal,
                                        0.15f * diagonal}});
        world.AddComponent(second, Rotation2D{rotation});
        world.AddComponent(second, RigidBody2D{});
        world.AddComponent(second, BoxCollider2D{{2.0f, 0.1f}});

        world.Update(1.0f / 60.0f);
        const auto* firstTransform = world.GetComponent<Transform2D>(first);
        const auto* secondTransform = world.GetComponent<Transform2D>(second);
        if (!firstTransform || !secondTransform ||
            (secondTransform->position - firstTransform->position)
                    .Dot({-diagonal, diagonal}) < 0.199f ||
            !physicsView->DidLastContactSolveConverge()) {
            return 59;
        }
        world.Shutdown();
    }

    // Verify swept SAT stops a fast rotated box at its first OBB contact.
    {
        using namespace StonePotatoes::Physics;
        constexpr float diagonal = 0.70710678118f;
        constexpr float rotation = 0.78539816339f;
        const StonePotatoes::Math::Vec2 normal{-diagonal, diagonal};
        StonePotatoes::ECS::World world;
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<Physics2DSystem>(StonePotatoes::Math::Vec2{}));
        world.Initialize();

        const auto wall = world.CreateEntity();
        world.AddComponent(wall, Transform2D{{0.0f, 0.0f}});
        world.AddComponent(wall, Rotation2D{rotation});
        RigidBody2D wallBody;
        wallBody.inverseMass = 0.0f;
        world.AddComponent(wall, wallBody);
        world.AddComponent(wall, BoxCollider2D{{2.0f, 0.1f}});

        const auto moving = world.CreateEntity();
        world.AddComponent(moving, Transform2D{normal * 5.0f});
        world.AddComponent(moving, Rotation2D{rotation});
        RigidBody2D movingBody;
        movingBody.velocity = normal * -100.0f;
        world.AddComponent(moving, movingBody);
        world.AddComponent(moving, BoxCollider2D{{2.0f, 0.1f}});

        world.Update(0.1f);
        const auto* transform = world.GetComponent<Transform2D>(moving);
        const auto* body = world.GetComponent<RigidBody2D>(moving);
        if (!transform || !body ||
            std::fabs(transform->position.Dot(normal) - 0.2f) > 0.001f ||
            std::fabs(body->velocity.Dot(normal)) > 0.0001f) {
            return 60;
        }
        world.Shutdown();
    }

    // The installed Physics target exposes torque-driven orientation updates.
    {
        using namespace StonePotatoes::Physics;
        StonePotatoes::ECS::World world;
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<Physics2DSystem>(StonePotatoes::Math::Vec2{}));
        world.Initialize();

        const auto entity = world.CreateEntity();
        world.AddComponent(entity, Transform2D{});
        world.AddComponent(entity, Rotation2D{0.25f});
        RigidBody2D body;
        body.angularVelocity = 0.5f;
        body.inverseInertia = 0.25f;
        if (!body.ApplyTorque(4.0f)) return 61;
        world.AddComponent(entity, body);
        world.Update(0.5f);

        const auto* rotation = world.GetComponent<Rotation2D>(entity);
        const auto* result = world.GetComponent<RigidBody2D>(entity);
        if (!rotation || !result ||
            std::fabs(rotation->radians - 0.75f) > 0.00001f ||
            std::fabs(result->angularVelocity - 1.0f) > 0.00001f ||
            result->accumulatedTorque != 0.0f) {
            return 62;
        }
        world.Shutdown();
    }

    // An off-center swept impact through the installed API generates spin.
    {
        using namespace StonePotatoes::Physics;
        StonePotatoes::ECS::World world;
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<Physics2DSystem>(StonePotatoes::Math::Vec2{}));
        world.Initialize();

        const auto wall = world.CreateEntity();
        world.AddComponent(wall, Transform2D{});
        RigidBody2D wallBody;
        wallBody.inverseMass = 0.0f;
        world.AddComponent(wall, wallBody);
        world.AddComponent(wall, BoxCollider2D{{0.5f, 0.5f}});

        const auto moving = world.CreateEntity();
        world.AddComponent(moving, Transform2D{{-1.2f, 0.4f}});
        world.AddComponent(moving, Rotation2D{});
        RigidBody2D body;
        body.velocity = {2.0f, 0.0f};
        body.inverseInertia = 1.0f;
        world.AddComponent(moving, body);
        world.AddComponent(moving, BoxCollider2D{{0.5f, 0.5f}});

        world.Update(0.2f);
        const auto* result = world.GetComponent<RigidBody2D>(moving);
        if (!result || std::fabs(result->angularVelocity) <= 0.05f)
            return 63;
        world.Shutdown();
    }

    // Swept rotational SAT catches a fast rotating bar crossing an obstacle.
    {
        using namespace StonePotatoes::Physics;
        StonePotatoes::ECS::World world;
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<Physics2DSystem>(StonePotatoes::Math::Vec2{}));
        world.Initialize();

        const auto rotating = world.CreateEntity();
        world.AddComponent(rotating, Transform2D{});
        world.AddComponent(rotating, Rotation2D{-1.57079632679f});
        RigidBody2D rotatingBody;
        rotatingBody.inverseMass = 0.0f;
        rotatingBody.inverseInertia = 1.0f;
        rotatingBody.angularVelocity = 16.0f * 3.14159265359f;
        world.AddComponent(rotating, rotatingBody);
        world.AddComponent(rotating, BoxCollider2D{{2.0f, 0.1f}});

        const auto obstacle = world.CreateEntity();
        world.AddComponent(obstacle, Transform2D{{1.5f, 0.0f}});
        RigidBody2D obstacleBody;
        obstacleBody.inverseMass = 0.0f;
        world.AddComponent(obstacle, obstacleBody);
        world.AddComponent(obstacle, BoxCollider2D{{0.1f, 0.1f}});

        world.Update(1.0f);
        const auto* rotation = world.GetComponent<Rotation2D>(rotating);
        const auto* result = world.GetComponent<RigidBody2D>(rotating);
        if (!rotation || !result || !std::isfinite(rotation->radians) ||
            !std::isfinite(result->angularVelocity) ||
            std::fabs(result->angularVelocity) >=
                std::fabs(rotatingBody.angularVelocity) ||
            std::fabs(rotation->radians -
                      (-1.57079632679f + rotatingBody.angularVelocity)) <=
                1.0f) {
            return 64;
        }
        world.Shutdown();
    }

    // The installed Physics target exposes optional restitution independently
    // of BoxCollider2D's persisted layout.
    {
        using namespace StonePotatoes::Physics;
        StonePotatoes::ECS::World world;
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<Physics2DSystem>(StonePotatoes::Math::Vec2{}));
        world.Initialize();

        const auto wall = world.CreateEntity();
        world.AddComponent(wall, Transform2D{{0.0f, 0.0f}});
        RigidBody2D wallBody;
        wallBody.inverseMass = 0.0f;
        world.AddComponent(wall, wallBody);
        world.AddComponent(wall, BoxCollider2D{{0.5f, 2.0f}});

        const auto bouncingBody = world.CreateEntity();
        world.AddComponent(bouncingBody, Transform2D{{-1.05f, 0.0f}});
        RigidBody2D body;
        body.velocity = {1.0f, 0.0f};
        world.AddComponent(bouncingBody, body);
        world.AddComponent(bouncingBody, BoxCollider2D{{0.5f, 0.5f}});
        world.AddComponent(bouncingBody, Restitution2D{0.5f});

        world.Update(0.1f);
        const auto* result = world.GetComponent<RigidBody2D>(bouncingBody);
        if (!result || std::fabs(result->velocity.x + 0.5f) > 0.0001f) {
            return 56;
        }
        world.Shutdown();
    }

    // Exercise the installed civilization stock-flow model using only its
    // public target. Transfers observe the same pre-step state, while an event
    // and KPI probe record the completed monthly step.
    {
        using StonePotatoes::Civilization::Economic::SDModel;
        using StonePotatoes::Civilization::Economic::SDState;

        SDModel economy;
        economy.AddStock("grain", 100.0f, 0.0f);
        economy.AddStock("food", 0.0f, 0.0f);
        economy.AddStock("treasury", 10.0f, 0.0f);
        economy.SetParam("grain_reserve", 20.0f);
        economy.SetDt(0.5f);
        economy.AddAux("available_grain", [](const SDState& state) {
            return state.S("grain") - state.P("grain_reserve");
        });
        economy.AddFlow("grain", [](const SDState& state) {
            return state.A("available_grain") >= 2.0f ? -2.0f : 0.0f;
        });
        economy.AddFlow("food", [](const SDState& state) {
            return state.A("available_grain") >= 2.0f ? 2.0f : 0.0f;
        });
        economy.AddEvent(0, "treasury", -3.0f);
        economy.AddProbe("stored_food", [](const SDState& state) {
            return state.S("grain") + state.S("food");
        });
        economy.Step();

        if (economy.Month() != 1 ||
            std::fabs(economy.Stock("grain") - 99.0f) > 0.0001f ||
            std::fabs(economy.Stock("food") - 1.0f) > 0.0001f ||
            std::fabs(economy.Stock("treasury") - 7.0f) > 0.0001f ||
            economy.History("grain").size() != 2 ||
            economy.ProbeHistory("stored_food").size() != 1 ||
            std::fabs(economy.ProbeHistory("stored_food").front() - 100.0f) >
                0.0001f) {
            return 48;
        }
    }

    // Exercise the installed first-pass demographic model.
    {
        const StonePotatoes::Civilization::Population::PopulationConfig config{
            100.0f, 1000.0f, 0.2f, 5.0f};
        auto population =
            StonePotatoes::Civilization::Population::BuildPopulationModel(
                config);
        population.Step();
        if (std::fabs(population.Stock("population") - 123.0f) > 0.001f ||
            population.History("population").size() != 2) {
            return 70;
        }
        population.SetParam("net_migration_per_month", -10.0f);
        population.Step();
        if (std::fabs(population.Stock("population") - 134.5742f) > 0.01f) {
            return 71;
        }
    }

    // Verify installed trade transfers conserve commodity and respect storage.
    {
        using namespace StonePotatoes::Civilization::Economic;
        TradeNetworkConfig config;
        config.initialStockByRegion = {100.0f, 20.0f};
        config.storageCapacityByRegion = {500.0f, 500.0f};
        config.monthlyProductionByRegion = {10.0f, 0.0f};
        config.monthlyConsumptionByRegion = {5.0f, 0.0f};
        config.monthlyTradeFromRegionToRegion = {
            {0.0f, 30.0f}, {0.0f, 0.0f}};
        auto trade = BuildTradeNetworkModel(config);
        trade.Step();
        const float sourceStock = trade.Stock(TradeStockName(0));
        const float destinationStock = trade.Stock(TradeStockName(1));
        if (std::fabs(sourceStock - 75.0f) > 0.001f ||
            std::fabs(destinationStock - 50.0f) > 0.001f ||
            std::fabs(sourceStock + destinationStock - 125.0f) > 0.001f) {
            return 80;
        }

        trade.SetParam(TradeRouteParameterName(0, 1), 0.0f);
        trade.SetParam(TradeRouteParameterName(1, 0), 10.0f);
        trade.SetParam(TradeProductionParameterName(1), 5.0f);
        trade.Step();
        if (std::fabs(trade.Stock(TradeStockName(0)) - 90.0f) > 0.001f ||
            std::fabs(trade.Stock(TradeStockName(1)) - 45.0f) > 0.001f) {
            return 82;
        }

        config.initialStockByRegion = {100.0f, 45.0f};
        config.storageCapacityByRegion = {1000.0f, 50.0f};
        config.monthlyProductionByRegion = {0.0f, 0.0f};
        config.monthlyConsumptionByRegion = {0.0f, 0.0f};
        config.monthlyTradeFromRegionToRegion = {
            {0.0f, 25.0f}, {0.0f, 0.0f}};
        trade = BuildTradeNetworkModel(config);
        trade.Step();
        if (std::fabs(trade.Stock(TradeStockName(0)) - 95.0f) >
                0.001f ||
            trade.Stock(TradeStockName(1)) != 50.0f) {
            return 81;
        }
    }

    // Verify installed explicit demographic rates and scenario adjustment.
    {
        namespace Population = StonePotatoes::Civilization::Population;
        const Population::DemographicPopulationConfig config{
            100.0f, 1000.0f, 0.02f, 0.005f, 3.0f};
        auto population = Population::BuildDemographicPopulationModel(config);
        population.Step();
        if (std::fabs(population.Stock("population") - 104.3f) > 0.001f) {
            return 72;
        }
        population.SetParam("birth_rate_per_month", 0.0f);
        population.SetParam("death_rate_per_month", 0.01f);
        population.SetParam("net_migration_per_month", 0.0f);
        population.Step();
        if (std::fabs(population.Stock("population") - 103.257f) > 0.01f) {
            return 73;
        }
    }

    // Verify installed regional migration conserves transfers and respects capacity.
    {
        namespace Population = StonePotatoes::Civilization::Population;
        Population::RegionalPopulationConfig config;
        config.initialPopulationByRegion = {100.0f, 50.0f};
        config.carryingCapacityByRegion = {1000.0f, 1000.0f};
        config.intrinsicGrowthRatePerMonthByRegion = {0.0f, 0.0f};
        config.monthlyMigrationFromRegionToRegion = {
            {0.0f, 20.0f}, {5.0f, 0.0f}};
        auto population = Population::BuildRegionalPopulationModel(config);
        population.Step();
        const float region0 = population.Stock(
            Population::RegionPopulationStockName(0));
        const float region1 = population.Stock(
            Population::RegionPopulationStockName(1));
        if (std::fabs(region0 - 85.0f) > 0.001f ||
            std::fabs(region1 - 65.0f) > 0.001f ||
            std::fabs(region0 + region1 - 150.0f) > 0.001f) {
            return 78;
        }

        config.initialPopulationByRegion = {100.0f, 90.0f};
        config.carryingCapacityByRegion = {1000.0f, 100.0f};
        config.monthlyMigrationFromRegionToRegion = {
            {0.0f, 30.0f}, {0.0f, 0.0f}};
        population = Population::BuildRegionalPopulationModel(config);
        population.Step();
        if (std::fabs(population.Stock(
                Population::RegionPopulationStockName(0)) - 90.0f) >
                0.001f ||
            population.Stock(Population::RegionPopulationStockName(1)) !=
                100.0f) {
            return 79;
        }
    }

    // Verify installed age-cohort transitions and global capacity-limited births.
    {
        namespace Population = StonePotatoes::Civilization::Population;
        Population::AgeStructuredPopulationConfig config;
        config.initialPopulationByAgeGroup = {100.0f, 50.0f, 10.0f};
        config.annualSurvivalRateByAgeGroup = {0.9f, 0.8f, 0.5f};
        config.annualBirthsPerPersonByAgeGroup = {0.0f, 0.2f, 0.1f};
        config.carryingCapacity = 1000.0f;
        auto population = Population::BuildAgeStructuredPopulationModel(config);
        population.Step();
        if (std::fabs(population.Stock(Population::AgeGroupStockName(0)) - 11.0f) >
                0.001f ||
            std::fabs(population.Stock(Population::AgeGroupStockName(1)) - 90.0f) >
                0.001f ||
            std::fabs(population.Stock(Population::AgeGroupStockName(2)) - 45.0f) >
                0.001f) {
            return 74;
        }
        config.initialPopulationByAgeGroup = {100.0f, 0.0f};
        config.annualSurvivalRateByAgeGroup = {1.0f, 1.0f};
        config.annualBirthsPerPersonByAgeGroup = {2.0f, 0.0f};
        config.carryingCapacity = 150.0f;
        population = Population::BuildAgeStructuredPopulationModel(config);
        population.Step();
        if (population.Stock(Population::AgeGroupStockName(0)) != 50.0f ||
            population.Stock(Population::AgeGroupStockName(1)) != 100.0f) {
            return 75;
        }

        config.initialPopulationByAgeGroup = {100.0f, 50.0f};
        config.annualSurvivalRateByAgeGroup = {1.0f, 1.0f};
        config.annualBirthsPerPersonByAgeGroup = {0.0f, 0.0f};
        config.annualNetMigrationByAgeGroup = {10.0f, -1000.0f};
        config.carryingCapacity = 1000.0f;
        population = Population::BuildAgeStructuredPopulationModel(config);
        population.Step();
        if (std::fabs(population.Stock(
                Population::AgeGroupStockName(0)) - 10.0f) > 0.001f ||
            std::fabs(population.Stock(
                Population::AgeGroupStockName(1))) > 0.001f) {
            return 76;
        }

        config.annualNetMigrationByAgeGroup = {1000.0f, 1000.0f};
        config.carryingCapacity = 160.0f;
        population = Population::BuildAgeStructuredPopulationModel(config);
        population.Step();
        if (std::fabs(population.Stock(
                Population::AgeGroupStockName(0)) - 5.0f) > 0.001f ||
            std::fabs(population.Stock(
                Population::AgeGroupStockName(1)) - 155.0f) > 0.001f) {
            return 77;
        }
    }

    // Exercise installed deterministic lockstep command and checksum packets.
    {
        using StonePotatoes::Networking::LockstepSync;

        LockstepSync::Config config;
        config.numPeers = 2;
        config.localPeerId = 0;
        config.inputDelayTicks = 2;
        LockstepSync sync(config);

        sync.QueueLocalInput(std::string("move"));
        const auto localPackets = sync.SealLocalPackets();
        if (localPackets.size() != 1 || localPackets.front().size() < 8) {
            return 49;
        }

        LockstepSync::TickCommands commands;
        for (int expectedTick = 0; expectedTick < 2; ++expectedTick) {
            if (!sync.ReadyToAdvance() || !sync.TryAdvance(commands) ||
                commands.tick != expectedTick || !commands.inputs.empty()) {
                return 50;
            }
        }
        if (sync.ReadyToAdvance()) {
            return 51;
        }

        const std::vector<uint8_t> remoteCommand{0x10, 0x20};
        const auto remotePacket = LockstepSync::EncodeCommandPacket(
            1, 2, {remoteCommand});
        if (!sync.OnPeerPacket(remotePacket)) {
            return 52;
        }

        constexpr uint32_t localChecksum = 0xAABBCCDD;
        sync.RecordLocalChecksum(2, localChecksum);
        const auto checksumPacket = LockstepSync::EncodeChecksumPacket(
            1, 2, 0x11223344);
        if (!sync.OnPeerPacket(checksumPacket) || !sync.ReadyToAdvance() ||
            !sync.TryAdvance(commands) || commands.tick != 2 ||
            commands.inputs.size() != 2 ||
            commands.inputs[0].first != 0 ||
            commands.inputs[0].second != std::vector<uint8_t>({'m', 'o', 'v', 'e'}) ||
            commands.inputs[1].first != 1 ||
            commands.inputs[1].second != remoteCommand ||
            sync.ChecksumMismatches(2) != std::vector<int>{1}) {
            return 53;
        }
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
