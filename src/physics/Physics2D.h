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

// Optional box orientation, expressed in radians. Physics2D integrates it
// when an entity also has a RigidBody2D with nonzero inverse inertia.
struct Rotation2D {
    float radians = 0.0f;
};

struct RigidBody2D {
    Math::Vec2 velocity{};
    Math::Vec2 accumulatedForce{};
    float inverseMass = 1.0f;
    float angularVelocity = 0.0f;
    float accumulatedTorque = 0.0f;
    // Zero locks rotation. OBB contact angular response requires Rotation2D.
    float inverseInertia = 0.0f;

    // Accumulate a finite force for the next physics tick.
    // 累積有限值的力，供下一個物理 tick 使用。
    bool ApplyForce(const Math::Vec2& force);
    // Accumulate a finite torque for the next physics tick.
    bool ApplyTorque(float torque);
};

// Box collider centered on Transform2D::position. An optional Rotation2D
// component gives it a fixed orientation; without one the box is axis-aligned.
// 以 Transform2D::position 為中心的方盒；可選 Rotation2D 指定固定朝向。
struct BoxCollider2D {
    Math::Vec2 halfExtents{0.5f, 0.5f};
    // Coulomb friction coefficients; zero keeps contacts frictionless.
    // 庫侖摩擦係數；預設為零以維持無摩擦接觸。
    float staticFriction = 0.0f;
    float dynamicFriction = 0.0f;
};

// Optional bounce material. Contacts combine two coefficients by their max;
// zero on both bodies preserves the existing inelastic behavior.
struct Restitution2D {
    float coefficient = 0.0f;
};

static_assert(std::is_trivially_copyable_v<Transform2D>);
static_assert(std::is_trivially_copyable_v<Rotation2D>);
static_assert(std::is_trivially_copyable_v<RigidBody2D>);
static_assert(std::is_trivially_copyable_v<BoxCollider2D>);
static_assert(std::is_trivially_copyable_v<Restitution2D>);

void RegisterPhysicsComponents(ECS::World& world);

// Integrates bodies with semi-implicit Euler, then performs bounded,
// entity-ID ordered iterations over discrete oriented-box contacts with
// optional restitution and Coulomb friction, plus event-driven swept SAT
// contacts with conservative advancement for rotating boxes.
// Use Game::SetFixedStep to feed this system a stable tick; inverseMass == 0
// disables translation and inverseInertia == 0 disables rotation. Contact
// impulses affect both velocities using an estimated OBB contact point;
// rotating swept SAT bounds per-pair advancement and recalculates orientation
// during the remaining tick after an impact.
// 以半隱式 Euler 積分剛體，再依 entity ID 順序有限輪解析離散 OBB SAT 接觸、
// 可選恢復係數與庫侖摩擦，並以事件推進方式處理固定朝向方盒的 swept SAT
// 碰撞。透過 Game::SetFixedStep 提供固定 tick；inverseMass == 0 停用平移，
// inverseInertia == 0 停用旋轉。接觸衝量會透過估算的 OBB 接觸點影響兩種速度；
// 旋轉 swept SAT 使用有界保守推進，碰撞後會依新角速度重算剩餘 tick 的朝向。
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
