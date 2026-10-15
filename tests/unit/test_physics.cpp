#include "game/Game.h"
#include "ecs/World.h"
#include "physics/Physics2D.h"

#include <cmath>
#include <limits>
#include <memory>
#include <vector>

using namespace StonePotatoes;
using namespace StonePotatoes::Physics;

namespace {

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (false)

struct EmptyScene final : Game::Scene {
    void Update(ECS::World&, float) override {}
};

struct Snapshot {
    Math::Vec2 position;
    Math::Vec2 velocity;
};

Snapshot Simulate(const std::vector<float>& frameDeltas) {
    ECS::World world;
    world.GetSystemScheduler().RegisterSystem(
        std::make_unique<Physics2DSystem>(Math::Vec2{0.0f, -8.0f}));
    world.Initialize();

    const ECS::Entity entity = world.CreateEntity();
    world.AddComponent(entity, Transform2D{{1.0f, 2.0f}});
    RigidBody2D body;
    body.velocity = {2.0f, 1.0f};
    body.inverseMass = 0.5f;
    body.ApplyForce({4.0f, 0.0f});
    world.AddComponent(entity, body);

    Game::Game game;
    game.SetScene(std::make_unique<EmptyScene>());
    game.SetFixedStep(0.125f);
    game.Start(world);
    for (float delta : frameDeltas) game.Update(delta);

    const Snapshot result{world.GetComponent<Transform2D>(entity)->position,
                          world.GetComponent<RigidBody2D>(entity)->velocity};
    game.Stop();
    world.Shutdown();
    return result;
}

Snapshot SimulateBoxCollision(const std::vector<float>& frameDeltas) {
    ECS::World world;
    world.GetSystemScheduler().RegisterSystem(
        std::make_unique<Physics2DSystem>(Math::Vec2{0.0f, 0.0f}));
    world.Initialize();

    const ECS::Entity fixed = world.CreateEntity();
    world.AddComponent(fixed, Transform2D{{0.0f, 0.0f}});
    RigidBody2D fixedBody;
    fixedBody.inverseMass = 0.0f;
    world.AddComponent(fixed, fixedBody);
    world.AddComponent(fixed, BoxCollider2D{{0.5f, 0.5f}});

    const ECS::Entity moving = world.CreateEntity();
    world.AddComponent(moving, Transform2D{{-1.5f, 0.0f}});
    RigidBody2D movingBody;
    movingBody.velocity = {1.0f, 0.0f};
    world.AddComponent(moving, movingBody);
    world.AddComponent(moving, BoxCollider2D{{0.5f, 0.5f}});

    Game::Game game;
    game.SetScene(std::make_unique<EmptyScene>());
    game.SetFixedStep(0.125f);
    game.Start(world);
    for (float delta : frameDeltas) game.Update(delta);

    const Snapshot result{world.GetComponent<Transform2D>(moving)->position,
                          world.GetComponent<RigidBody2D>(moving)->velocity};
    game.Stop();
    world.Shutdown();
    return result;
}

int TestSemiImplicitIntegrationAndStaticBodies() {
    ECS::World world;
    RegisterPhysicsComponents(world);
    world.GetSystemScheduler().RegisterSystem(
        std::make_unique<Physics2DSystem>(Math::Vec2{0.0f, -8.0f}));
    world.Initialize();

    const ECS::Entity moving = world.CreateEntity();
    world.AddComponent(moving, Transform2D{});
    RigidBody2D movingBody;
    movingBody.inverseMass = 0.5f;
    CHECK(movingBody.ApplyForce({4.0f, 0.0f}));
    world.AddComponent(moving, movingBody);

    const ECS::Entity fixed = world.CreateEntity();
    world.AddComponent(fixed, Transform2D{{2.0f, 3.0f}});
    RigidBody2D fixedBody;
    fixedBody.velocity = {3.0f, 4.0f};
    fixedBody.accumulatedForce = {8.0f, 2.0f};
    fixedBody.inverseMass = 0.0f;
    world.AddComponent(fixed, fixedBody);

    Game::Game game;
    game.SetScene(std::make_unique<EmptyScene>());
    CHECK(game.SetFixedStep(0.125f));
    CHECK(game.Start(world));
    game.Update(0.0625f);
    CHECK(world.GetComponent<Transform2D>(moving)->position.x == 0.0f);

    game.Update(0.0625f);
    const auto* firstPosition = world.GetComponent<Transform2D>(moving);
    const auto* firstBody = world.GetComponent<RigidBody2D>(moving);
    CHECK(firstPosition->position.x == 0.03125f);
    CHECK(firstPosition->position.y == -0.125f);
    CHECK(firstBody->velocity.x == 0.25f);
    CHECK(firstBody->velocity.y == -1.0f);
    CHECK(firstBody->accumulatedForce.x == 0.0f &&
          firstBody->accumulatedForce.y == 0.0f);

    game.Update(0.125f);
    const auto* secondPosition = world.GetComponent<Transform2D>(moving);
    const auto* secondBody = world.GetComponent<RigidBody2D>(moving);
    CHECK(secondPosition->position.x == 0.0625f);
    CHECK(secondPosition->position.y == -0.375f);
    CHECK(secondBody->velocity.x == 0.25f);
    CHECK(secondBody->velocity.y == -2.0f);

    const auto* fixedPosition = world.GetComponent<Transform2D>(fixed);
    const auto* fixedBodyAfterTick = world.GetComponent<RigidBody2D>(fixed);
    CHECK(fixedPosition->position.x == 2.0f &&
          fixedPosition->position.y == 3.0f);
    CHECK(fixedBodyAfterTick->velocity.x == 3.0f &&
          fixedBodyAfterTick->velocity.y == 4.0f);
    CHECK(fixedBodyAfterTick->accumulatedForce.x == 0.0f &&
          fixedBodyAfterTick->accumulatedForce.y == 0.0f);

    world.Update(std::numeric_limits<float>::quiet_NaN());
    world.Update(-0.125f);
    CHECK(secondPosition->position.x == 0.0625f &&
          secondPosition->position.y == -0.375f);
    CHECK(secondBody->velocity.x == 0.25f && secondBody->velocity.y == -2.0f);

    game.Stop();
    world.Shutdown();
    return 0;
}

int TestFramePartitionDeterminism() {
    const Snapshot byLargeFrames = Simulate(std::vector<float>(8, 0.125f));
    const Snapshot bySmallFrames = Simulate(std::vector<float>(16, 0.0625f));
    CHECK(byLargeFrames.position.x == bySmallFrames.position.x);
    CHECK(byLargeFrames.position.y == bySmallFrames.position.y);
    CHECK(byLargeFrames.velocity.x == bySmallFrames.velocity.x);
    CHECK(byLargeFrames.velocity.y == bySmallFrames.velocity.y);
    return 0;
}

int TestStaticAabbContactAndFramePartitionDeterminism() {
    const Snapshot byLargeFrames =
        SimulateBoxCollision(std::vector<float>(8, 0.125f));
    const Snapshot bySmallFrames =
        SimulateBoxCollision(std::vector<float>(16, 0.0625f));
    CHECK(byLargeFrames.position.x == -1.0f);
    CHECK(byLargeFrames.position.y == 0.0f);
    CHECK(byLargeFrames.velocity.x == 0.0f);
    CHECK(byLargeFrames.velocity.y == 0.0f);
    CHECK(byLargeFrames.position.x == bySmallFrames.position.x);
    CHECK(byLargeFrames.position.y == bySmallFrames.position.y);
    CHECK(byLargeFrames.velocity.x == bySmallFrames.velocity.x);
    CHECK(byLargeFrames.velocity.y == bySmallFrames.velocity.y);
    return 0;
}

int TestDynamicHeadOnContact() {
    ECS::World world;
    RegisterPhysicsComponents(world);
    world.GetSystemScheduler().RegisterSystem(
        std::make_unique<Physics2DSystem>(Math::Vec2{0.0f, 0.0f}));
    world.Initialize();

    const ECS::Entity left = world.CreateEntity();
    world.AddComponent(left, Transform2D{{-0.5f, 0.0f}});
    RigidBody2D leftBody;
    leftBody.velocity = {1.0f, 0.0f};
    world.AddComponent(left, leftBody);
    world.AddComponent(left, BoxCollider2D{{0.5f, 0.5f}});

    const ECS::Entity right = world.CreateEntity();
    world.AddComponent(right, Transform2D{{0.5f, 0.0f}});
    RigidBody2D rightBody;
    rightBody.velocity = {-1.0f, 0.0f};
    world.AddComponent(right, rightBody);
    world.AddComponent(right, BoxCollider2D{{0.5f, 0.5f}});

    Game::Game game;
    game.SetScene(std::make_unique<EmptyScene>());
    CHECK(game.SetFixedStep(0.25f));
    CHECK(game.Start(world));
    game.Update(0.25f);

    CHECK(world.GetComponent<Transform2D>(left)->position.x == -0.5f);
    CHECK(world.GetComponent<Transform2D>(right)->position.x == 0.5f);
    CHECK(world.GetComponent<RigidBody2D>(left)->velocity.x == 0.0f);
    CHECK(world.GetComponent<RigidBody2D>(right)->velocity.x == 0.0f);

    game.Stop();
    world.Shutdown();
    return 0;
}

int TestSpatialHashPrunesSeparatedPairsAndKeepsLargeColliders() {
    ECS::World world;
    auto physics = std::make_unique<Physics2DSystem>(Math::Vec2{0.0f, 0.0f});
    Physics2DSystem* physicsView = physics.get();
    world.GetSystemScheduler().RegisterSystem(std::move(physics));
    world.Initialize();

    constexpr int kSeparatedBodyCount = 256;
    for (int i = 0; i < kSeparatedBodyCount; ++i) {
        const ECS::Entity entity = world.CreateEntity();
        world.AddComponent(entity,
                           Transform2D{{static_cast<float>(i * 4), 0.0f}});
        RigidBody2D body;
        body.inverseMass = 0.0f;
        world.AddComponent(entity, body);
        world.AddComponent(entity, BoxCollider2D{{0.5f, 0.5f}});
    }

    const ECS::Entity large = world.CreateEntity();
    world.AddComponent(large, Transform2D{{2000.0f, 0.0f}});
    RigidBody2D largeBody;
    largeBody.inverseMass = 0.0f;
    world.AddComponent(large, largeBody);
    world.AddComponent(large, BoxCollider2D{{100.0f, 100.0f}});

    const ECS::Entity insideLarge = world.CreateEntity();
    world.AddComponent(insideLarge, Transform2D{{2099.75f, 0.0f}});
    world.AddComponent(insideLarge, RigidBody2D{});
    world.AddComponent(insideLarge, BoxCollider2D{{0.5f, 0.5f}});

    world.Update(0.125f);

    const std::size_t allPairs =
        static_cast<std::size_t>(kSeparatedBodyCount + 2) *
        static_cast<std::size_t>(kSeparatedBodyCount + 1) / 2;
    CHECK(physicsView->GetLastBroadPhasePairCount() < allPairs / 8);
    CHECK(world.GetComponent<Transform2D>(insideLarge)->position.x == 2100.5f);

    world.Shutdown();
    return 0;
}

int TestInvalidInputsAreIgnored() {
    RigidBody2D body;
    CHECK(!body.ApplyForce(
        {std::numeric_limits<float>::quiet_NaN(), 1.0f}));
    CHECK(body.accumulatedForce.x == 0.0f && body.accumulatedForce.y == 0.0f);
    CHECK(body.ApplyForce({1.0f, -2.0f}));
    CHECK(!body.ApplyForce(
        {std::numeric_limits<float>::infinity(), 0.0f}));
    CHECK(body.accumulatedForce.x == 1.0f && body.accumulatedForce.y == -2.0f);
    return 0;
}

} // namespace

int main() {
    if (const int result = TestSemiImplicitIntegrationAndStaticBodies())
        return result;
    if (const int result = TestFramePartitionDeterminism()) return result;
    if (const int result = TestStaticAabbContactAndFramePartitionDeterminism())
        return result;
    if (const int result = TestDynamicHeadOnContact()) return result;
    if (const int result =
            TestSpatialHashPrunesSeparatedPairsAndKeepsLargeColliders())
        return result;
    if (const int result = TestInvalidInputsAreIgnored()) return result;
    return 0;
}
