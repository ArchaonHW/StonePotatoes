#pragma once

#include "ecs/system/System.h"
#include "math/Vec2.h"

#include <cstddef>
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

// Axis-aligned box collider centered on Transform2D::position.
// 以 Transform2D::position 為中心的軸對齊方盒碰撞體。
struct BoxCollider2D {
    Math::Vec2 halfExtents{0.5f, 0.5f};
    // Coulomb friction coefficients; zero keeps contacts frictionless.
    // 庫侖摩擦係數；預設為零以維持無摩擦接觸。
    float staticFriction = 0.0f;
    float dynamicFriction = 0.0f;
};

static_assert(std::is_trivially_copyable_v<Transform2D>);
static_assert(std::is_trivially_copyable_v<RigidBody2D>);
static_assert(std::is_trivially_copyable_v<BoxCollider2D>);

void RegisterPhysicsComponents(ECS::World& world);

// Integrates bodies with semi-implicit Euler, then performs bounded,
// entity-ID ordered iterations over discrete zero-restitution AABB contacts
// with Coulomb friction, plus swept-AABB contacts against static boxes.
// Use Game::SetFixedStep to feed this system a stable tick; inverseMass == 0
// marks an immovable body. Rotated shapes and dynamic-to-dynamic continuous
// collision detection are intentionally outside this slice.
// 以半隱式 Euler 積分剛體，再依 entity ID 順序有限輪解析離散、零恢復係數
// AABB 接觸與庫侖摩擦，另對靜態方盒進行 swept-AABB 碰撞。透過
// Game::SetFixedStep 提供固定 tick；inverseMass == 0 代表不可移動的剛體。
// 旋轉及動態剛體間的連續碰撞偵測不在此切片範圍。
class Physics2DSystem final : public ECS::System {
public:
    explicit Physics2DSystem(Math::Vec2 gravity = {0.0f, -9.81f});

    void Initialize(ECS::World* world) override;
    void Update(ECS::World* world, float deltaTime) override;
    void DescribeAccess(ECS::AccessSpec& access) const override;

    Math::Vec2 GetGravity() const { return m_Gravity; }
    // Unique broad-phase pairs sent to the AABB narrow phase in the last tick.
    // 上一個 tick 送入 AABB 窄階段的唯一 broad-phase 配對數。
    std::size_t GetLastBroadPhasePairCount() const {
        return m_LastBroadPhasePairCount;
    }
    std::size_t GetLastContactIterationCount() const {
        return m_LastContactIterationCount;
    }
    bool DidLastContactSolveConverge() const {
        return m_LastContactSolveConverged;
    }

private:
    Math::Vec2 m_Gravity;
    std::size_t m_LastBroadPhasePairCount = 0;
    std::size_t m_LastContactIterationCount = 0;
    bool m_LastContactSolveConverged = true;
};

} // namespace StonePotatoes::Physics
