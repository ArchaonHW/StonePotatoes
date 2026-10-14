#pragma once

#include "ecs/system/System.h"
#include "math/Vec2.h"

#include <type_traits>

namespace StonePotatoes::ECS {
class World;
}

namespace StonePotatoes::Physics {

// ECS components for deterministic 2D force integration.
// 確定性 2D 力積分所用的 ECS 組件。
struct Transform2D {
    Math::Vec2 position{};
};

struct RigidBody2D {
    Math::Vec2 velocity{};
    Math::Vec2 accumulatedForce{};
    float inverseMass = 1.0f;

    // Accumulate a finite force for the next physics tick.
    // 累積有限值的力，供下一個物理 tick 使用。
    bool ApplyForce(const Math::Vec2& force);
};

static_assert(std::is_trivially_copyable_v<Transform2D>);
static_assert(std::is_trivially_copyable_v<RigidBody2D>);

void RegisterPhysicsComponents(ECS::World& world);

// Integrates bodies with semi-implicit Euler. Use Game::SetFixedStep to feed
// this system a stable tick; inverseMass == 0 marks an immovable body.
// 以半隱式 Euler 積分剛體。透過 Game::SetFixedStep 提供固定 tick；
// inverseMass == 0 代表不可移動的剛體。
class Physics2DSystem final : public ECS::System {
public:
    explicit Physics2DSystem(Math::Vec2 gravity = {0.0f, -9.81f});

    void Initialize(ECS::World* world) override;
    void Update(ECS::World* world, float deltaTime) override;
    void DescribeAccess(ECS::AccessSpec& access) const override;

    Math::Vec2 GetGravity() const { return m_Gravity; }

private:
    Math::Vec2 m_Gravity;
};

} // namespace StonePotatoes::Physics
