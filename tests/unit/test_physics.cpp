#include "game/Game.h"
#include "ecs/World.h"
#include "physics/Physics2D.h"

#include <cmath>
#include <cstdint>
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

int TestCoulombContactFriction() {
    ECS::World world;
    RegisterPhysicsComponents(world);
    world.GetSystemScheduler().RegisterSystem(
        std::make_unique<Physics2DSystem>(Math::Vec2{0.0f, 0.0f}));
    world.Initialize();

    struct ContactPair {
        ECS::Entity moving;
        float expectedTangentialVelocity;
    };
    std::vector<ContactPair> pairs;
    const auto addPair = [&world, &pairs](float y, float tangentSpeed,
                                         float expectedTangentialVelocity) {
        const ECS::Entity fixed = world.CreateEntity();
        world.AddComponent(fixed, Transform2D{{0.0f, y}});
        RigidBody2D fixedBody;
        fixedBody.inverseMass = 0.0f;
        world.AddComponent(fixed, fixedBody);
        world.AddComponent(fixed, BoxCollider2D{{0.5f, 0.5f}, 1.0f, 0.5f});

        const ECS::Entity moving = world.CreateEntity();
        world.AddComponent(moving, Transform2D{{-0.75f, y}});
        RigidBody2D movingBody;
        movingBody.velocity = {1.0f, tangentSpeed};
        world.AddComponent(moving, movingBody);
        world.AddComponent(moving,
                           BoxCollider2D{{0.5f, 0.5f}, 1.0f, 0.5f});
        pairs.push_back({moving, expectedTangentialVelocity});
    };

    // The first body exceeds the static-friction limit and receives kinetic
    // friction; the second remains within the limit and sticks.
    addPair(0.0f, 4.0f, 3.5f);
    addPair(8.0f, 0.25f, 0.0f);
    world.Update(0.125f);

    for (const ContactPair& pair : pairs) {
        const auto* transform = world.GetComponent<Transform2D>(pair.moving);
        const auto* body = world.GetComponent<RigidBody2D>(pair.moving);
        CHECK(transform->position.x == -1.0f);
        CHECK(body->velocity.x == 0.0f);
        CHECK(std::fabs(body->velocity.y - pair.expectedTangentialVelocity) <
              0.00001f);
    }

    world.Shutdown();
    return 0;
}

int TestRestitutionForDiscreteAndSweptContacts() {
    const auto simulate = [](bool swept) {
        ECS::World world;
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<Physics2DSystem>(Math::Vec2{0.0f, 0.0f}));
        world.Initialize();

        const ECS::Entity wall = world.CreateEntity();
        world.AddComponent(wall, Transform2D{{0.0f, 0.0f}});
        RigidBody2D wallBody;
        wallBody.inverseMass = 0.0f;
        world.AddComponent(wall, wallBody);
        world.AddComponent(wall, BoxCollider2D{{0.5f, 2.0f}});

        const ECS::Entity moving = world.CreateEntity();
        world.AddComponent(moving,
                           Transform2D{{swept ? -5.0f : -1.05f, 0.0f}});
        RigidBody2D movingBody;
        movingBody.velocity = {swept ? 100.0f : 1.0f, 0.0f};
        world.AddComponent(moving, movingBody);
        world.AddComponent(moving, BoxCollider2D{{0.5f, 0.5f}});
        world.AddComponent(moving, Restitution2D{0.5f});

        world.Update(0.1f);
        const Snapshot result{
            world.GetComponent<Transform2D>(moving)->position,
            world.GetComponent<RigidBody2D>(moving)->velocity};
        world.Shutdown();
        return result;
    };

    const Snapshot discrete = simulate(false);
    CHECK(std::isfinite(discrete.position.x));
    CHECK(std::fabs(discrete.velocity.x + 0.5f) < 0.0001f);

    const Snapshot swept = simulate(true);
    CHECK(std::fabs(swept.position.x + 4.0f) < 0.001f);
    CHECK(std::fabs(swept.velocity.x + 50.0f) < 0.0001f);
    return 0;
}

int TestInvalidRestitutionSkipsCollision() {
    ECS::World world;
    world.GetSystemScheduler().RegisterSystem(
        std::make_unique<Physics2DSystem>(Math::Vec2{0.0f, 0.0f}));
    world.Initialize();

    const ECS::Entity wall = world.CreateEntity();
    world.AddComponent(wall, Transform2D{{0.0f, 0.0f}});
    RigidBody2D wallBody;
    wallBody.inverseMass = 0.0f;
    world.AddComponent(wall, wallBody);
    world.AddComponent(wall, BoxCollider2D{{0.5f, 2.0f}});

    const ECS::Entity moving = world.CreateEntity();
    world.AddComponent(moving, Transform2D{{-1.05f, 0.0f}});
    RigidBody2D movingBody;
    movingBody.velocity = {1.0f, 0.0f};
    world.AddComponent(moving, movingBody);
    world.AddComponent(moving, BoxCollider2D{{0.5f, 0.5f}});
    world.AddComponent(moving, Restitution2D{1.1f});

    world.Update(0.1f);
    CHECK(std::fabs(world.GetComponent<Transform2D>(moving)->position.x +
                    0.95f) < 0.0001f);
    CHECK(world.GetComponent<RigidBody2D>(moving)->velocity.x == 1.0f);
    world.Shutdown();
    return 0;
}

int TestContinuousCollisionAgainstStaticWalls() {
    ECS::World world;
    auto physics = std::make_unique<Physics2DSystem>(Math::Vec2{0.0f, 0.0f});
    Physics2DSystem* physicsView = physics.get();
    world.GetSystemScheduler().RegisterSystem(std::move(physics));
    world.Initialize();

    const ECS::Entity leftWall = world.CreateEntity();
    world.AddComponent(leftWall, Transform2D{{0.0f, 0.0f}});
    RigidBody2D leftWallBody;
    leftWallBody.inverseMass = 0.0f;
    world.AddComponent(leftWall, leftWallBody);
    world.AddComponent(leftWall, BoxCollider2D{{0.5f, 0.5f}});

    const ECS::Entity movingRight = world.CreateEntity();
    world.AddComponent(movingRight, Transform2D{{-5.0f, 0.0f}});
    RigidBody2D movingRightBody;
    movingRightBody.velocity = {100.0f, 5.0f};
    world.AddComponent(movingRight, movingRightBody);
    world.AddComponent(movingRight, BoxCollider2D{{0.5f, 0.5f}});

    const ECS::Entity rightWall = world.CreateEntity();
    world.AddComponent(rightWall, Transform2D{{0.0f, 10.0f}});
    RigidBody2D rightWallBody;
    rightWallBody.inverseMass = 0.0f;
    world.AddComponent(rightWall, rightWallBody);
    world.AddComponent(rightWall, BoxCollider2D{{0.5f, 0.5f}});

    const ECS::Entity movingLeft = world.CreateEntity();
    world.AddComponent(movingLeft, Transform2D{{5.0f, 10.0f}});
    RigidBody2D movingLeftBody;
    movingLeftBody.velocity = {-100.0f, 0.0f};
    world.AddComponent(movingLeft, movingLeftBody);
    world.AddComponent(movingLeft, BoxCollider2D{{0.5f, 0.5f}});

    world.Update(0.1f);

    const auto* leftTransform = world.GetComponent<Transform2D>(movingRight);
    const auto* leftBody = world.GetComponent<RigidBody2D>(movingRight);
    CHECK(std::fabs(leftTransform->position.x + 1.0f) < 0.0001f);
    CHECK(std::fabs(leftTransform->position.y - 0.5f) < 0.0001f);
    CHECK(leftBody->velocity.x == 0.0f);
    CHECK(leftBody->velocity.y == 5.0f);

    const auto* rightTransform = world.GetComponent<Transform2D>(movingLeft);
    const auto* rightBody = world.GetComponent<RigidBody2D>(movingLeft);
    CHECK(std::fabs(rightTransform->position.x - 1.0f) < 0.0001f);
    CHECK(rightBody->velocity.x == 0.0f);
    CHECK(rightBody->velocity.y == 0.0f);
    CHECK(physicsView->GetLastBroadPhasePairCount() >= 2);

    world.Shutdown();
    return 0;
}

int TestContinuousDynamicHeadOnContacts() {
    ECS::World world;
    auto physics = std::make_unique<Physics2DSystem>(Math::Vec2{0.0f, 0.0f});
    world.GetSystemScheduler().RegisterSystem(std::move(physics));
    world.Initialize();

    const auto addPair = [&world](float y, float inverseMassA,
                                  float inverseMassB) {
        const ECS::Entity a = world.CreateEntity();
        world.AddComponent(a, Transform2D{{-5.0f, y}});
        RigidBody2D bodyA;
        bodyA.velocity = {100.0f, 0.0f};
        bodyA.inverseMass = inverseMassA;
        world.AddComponent(a, bodyA);
        world.AddComponent(a, BoxCollider2D{{0.5f, 0.5f}});

        const ECS::Entity b = world.CreateEntity();
        world.AddComponent(b, Transform2D{{5.0f, y}});
        RigidBody2D bodyB;
        bodyB.velocity = {-100.0f, 0.0f};
        bodyB.inverseMass = inverseMassB;
        world.AddComponent(b, bodyB);
        world.AddComponent(b, BoxCollider2D{{0.5f, 0.5f}});
        return std::pair{a, b};
    };

    const auto equalMassPair = addPair(0.0f, 1.0f, 1.0f);
    const auto unequalMassPair = addPair(10.0f, 1.0f, 0.5f);
    world.Update(0.1f);

    const auto* equalA = world.GetComponent<Transform2D>(equalMassPair.first);
    const auto* equalB = world.GetComponent<Transform2D>(equalMassPair.second);
    CHECK(std::fabs(equalA->position.x + 0.5f) < 0.0001f);
    CHECK(std::fabs(equalB->position.x - 0.5f) < 0.0001f);
    CHECK(world.GetComponent<RigidBody2D>(equalMassPair.first)->velocity.x ==
          0.0f);
    CHECK(world.GetComponent<RigidBody2D>(equalMassPair.second)->velocity.x ==
          0.0f);

    const auto* unequalA =
        world.GetComponent<Transform2D>(unequalMassPair.first);
    const auto* unequalB =
        world.GetComponent<Transform2D>(unequalMassPair.second);
    const auto* unequalBodyA =
        world.GetComponent<RigidBody2D>(unequalMassPair.first);
    const auto* unequalBodyB =
        world.GetComponent<RigidBody2D>(unequalMassPair.second);
    CHECK(std::fabs(unequalA->position.x + 7.0f / 3.0f) < 0.0002f);
    CHECK(std::fabs(unequalB->position.x + 4.0f / 3.0f) < 0.0002f);
    CHECK(std::fabs(unequalBodyA->velocity.x + 100.0f / 3.0f) < 0.0001f);
    CHECK(std::fabs(unequalBodyB->velocity.x + 100.0f / 3.0f) < 0.0001f);

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

int TestGridBoundaryAndNegativeCoordinateContacts() {
    ECS::World world;
    auto physics = std::make_unique<Physics2DSystem>(Math::Vec2{0.0f, 0.0f});
    Physics2DSystem* physicsView = physics.get();
    world.GetSystemScheduler().RegisterSystem(std::move(physics));
    world.Initialize();

    struct ExpectedBody {
        ECS::Entity entity;
        float x;
        float y;
    };
    std::vector<ExpectedBody> expected;
    const auto addPair = [&world, &expected](Math::Vec2 fixedPosition,
                                             float direction) {
        const ECS::Entity fixed = world.CreateEntity();
        world.AddComponent(fixed, Transform2D{fixedPosition});
        RigidBody2D fixedBody;
        fixedBody.inverseMass = 0.0f;
        world.AddComponent(fixed, fixedBody);
        world.AddComponent(fixed, BoxCollider2D{{0.5f, 0.5f}});

        const ECS::Entity moving = world.CreateEntity();
        world.AddComponent(
            moving,
            Transform2D{{fixedPosition.x + direction * 0.75f,
                         fixedPosition.y}});
        RigidBody2D movingBody;
        movingBody.velocity = {-direction, 0.0f};
        world.AddComponent(moving, movingBody);
        world.AddComponent(moving, BoxCollider2D{{0.5f, 0.5f}});
        expected.push_back(
            {moving, fixedPosition.x + direction, fixedPosition.y});
    };

    // These boxes touch the +2 and -2 grid boundaries exactly.
    addPair({1.5f, 0.0f}, 1.0f);
    addPair({-1.5f, 32.0f}, -1.0f);

    // Deterministic pseudo-random layout of isolated contacts across positive
    // and negative grid coordinates. The fixed seed keeps expected results
    // reproducible across platforms and test runs.
    std::uint32_t state = 0x12345678u;
    const auto nextValue = [&state]() {
        state = state * 1664525u + 1013904223u;
        return state;
    };
    for (int i = 0; i < 64; ++i) {
        const int column = i % 8;
        const int row = i / 8;
        const int xBand = column < 4 ? -256 : 256;
        const int yBand = row < 4 ? -256 : 256;
        const float centerX = static_cast<float>(
            (column < 4 ? column : column - 4) * 16 + xBand) +
            ((nextValue() & 1u) ? 1.5f : -1.5f);
        const float centerY = static_cast<float>(
            (row < 4 ? row : row - 4) * 16 + yBand) +
            ((nextValue() & 1u) ? 1.5f : -1.5f);
        const float direction = (nextValue() & 1u) ? 1.0f : -1.0f;
        addPair({centerX, centerY}, direction);
    }

    world.Update(0.125f);

    CHECK(physicsView->GetLastBroadPhasePairCount() == expected.size());
    for (const ExpectedBody& result : expected) {
        const auto* transform = world.GetComponent<Transform2D>(result.entity);
        const auto* body = world.GetComponent<RigidBody2D>(result.entity);
        CHECK(transform->position.x == result.x);
        CHECK(transform->position.y == result.y);
        CHECK(body->velocity.x == 0.0f);
        CHECK(body->velocity.y == 0.0f);
    }

    world.Shutdown();
    return 0;
}

int TestMultiContactIterationsDiscoverNewContacts() {
    ECS::World world;
    auto physics = std::make_unique<Physics2DSystem>(Math::Vec2{0.0f, 0.0f});
    Physics2DSystem* physicsView = physics.get();
    world.GetSystemScheduler().RegisterSystem(std::move(physics));
    world.Initialize();

    const ECS::Entity floor = world.CreateEntity();
    world.AddComponent(floor, Transform2D{{0.0f, 0.9f}});
    RigidBody2D floorBody;
    floorBody.inverseMass = 0.0f;
    world.AddComponent(floor, floorBody);
    world.AddComponent(floor, BoxCollider2D{{0.5f, 0.5f}});

    const ECS::Entity lower = world.CreateEntity();
    world.AddComponent(lower, Transform2D{{0.0f, 1.4f}});
    world.AddComponent(lower, RigidBody2D{});
    world.AddComponent(lower, BoxCollider2D{{0.5f, 0.5f}});

    const ECS::Entity upper = world.CreateEntity();
    world.AddComponent(upper, Transform2D{{0.0f, 2.55f}});
    world.AddComponent(upper, RigidBody2D{});
    world.AddComponent(upper, BoxCollider2D{{0.5f, 0.5f}});

    // The lower body initially overlaps the floor. The upper body's AABB is
    // in another grid cell with a small gap, so only positional correction
    // creates the lower/upper contact for a later solver iteration to find.
    world.Update(0.125f);

    const float lowerY = world.GetComponent<Transform2D>(lower)->position.y;
    const float upperY = world.GetComponent<Transform2D>(upper)->position.y;
    CHECK(physicsView->GetLastBroadPhasePairCount() == 2);
    CHECK(physicsView->GetLastContactIterationCount() > 1);
    CHECK(physicsView->GetLastContactIterationCount() <= 16);
    CHECK(physicsView->DidLastContactSolveConverge());
    CHECK(lowerY >= 1.89f && lowerY <= 1.9f);
    CHECK(upperY >= 2.89f && upperY <= 2.9f);
    CHECK(world.GetComponent<RigidBody2D>(lower)->velocity.y == 0.0f);
    CHECK(world.GetComponent<RigidBody2D>(upper)->velocity.y == 0.0f);

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
    if (const int result = TestCoulombContactFriction()) return result;
    if (const int result = TestRestitutionForDiscreteAndSweptContacts())
        return result;
    if (const int result = TestInvalidRestitutionSkipsCollision())
        return result;
    if (const int result = TestContinuousCollisionAgainstStaticWalls())
        return result;
    if (const int result = TestContinuousDynamicHeadOnContacts())
        return result;
    if (const int result =
            TestSpatialHashPrunesSeparatedPairsAndKeepsLargeColliders())
        return result;
    if (const int result = TestGridBoundaryAndNegativeCoordinateContacts())
        return result;
    if (const int result = TestMultiContactIterationsDiscoverNewContacts())
        return result;
    if (const int result = TestInvalidInputsAreIgnored()) return result;
    return 0;
}
