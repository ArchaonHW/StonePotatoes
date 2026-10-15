#include "Physics2D.h"

#include "ecs/World.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <vector>

namespace StonePotatoes::Physics {
namespace {

bool IsFinite(const Math::Vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

struct CollisionBody {
    ECS::Entity entity;
    Transform2D* transform;
    RigidBody2D* body;
    BoxCollider2D* collider;
};

bool IsValidCollisionBody(const Transform2D& transform,
                          const RigidBody2D& body,
                          const BoxCollider2D& collider) {
    return IsFinite(transform.position) && IsFinite(body.velocity) &&
           IsFinite(body.accumulatedForce) &&
           std::isfinite(body.inverseMass) && body.inverseMass >= 0.0f &&
           IsFinite(collider.halfExtents) && collider.halfExtents.x > 0.0f &&
           collider.halfExtents.y > 0.0f;
}

void ResolvePair(CollisionBody& a, CollisionBody& b) {
    const float dx = b.transform->position.x - a.transform->position.x;
    const float dy = b.transform->position.y - a.transform->position.y;
    const float combinedHalfWidth = a.collider->halfExtents.x +
                                    b.collider->halfExtents.x;
    const float combinedHalfHeight = a.collider->halfExtents.y +
                                     b.collider->halfExtents.y;
    if (!std::isfinite(dx) || !std::isfinite(dy) ||
        !std::isfinite(combinedHalfWidth) ||
        !std::isfinite(combinedHalfHeight)) {
        return;
    }

    const float overlapX = combinedHalfWidth - std::fabs(dx);
    const float overlapY = combinedHalfHeight - std::fabs(dy);
    if (overlapX <= 0.0f || overlapY <= 0.0f) return;

    // Resolve the axis with the least penetration. Ties use X, and coincident
    // centers point from the lower entity ID toward the higher entity ID.
    const bool useX = overlapX <= overlapY;
    const float penetration = useX ? overlapX : overlapY;
    const float signedDelta = useX ? dx : dy;
    const Math::Vec2 normal = useX
        ? Math::Vec2{signedDelta < 0.0f ? -1.0f : 1.0f, 0.0f}
        : Math::Vec2{0.0f, signedDelta < 0.0f ? -1.0f : 1.0f};

    const float inverseMassSum = a.body->inverseMass + b.body->inverseMass;
    if (!std::isfinite(inverseMassSum) || inverseMassSum <= 0.0f) return;

    const float aShare = a.body->inverseMass / inverseMassSum;
    const float bShare = b.body->inverseMass / inverseMassSum;
    const Math::Vec2 correctionA = normal * (-penetration * aShare);
    const Math::Vec2 correctionB = normal * (penetration * bShare);
    const Math::Vec2 positionA = a.transform->position + correctionA;
    const Math::Vec2 positionB = b.transform->position + correctionB;

    const Math::Vec2 velocityAForContact = a.body->inverseMass > 0.0f
        ? a.body->velocity : Math::Vec2{};
    const Math::Vec2 velocityBForContact = b.body->inverseMass > 0.0f
        ? b.body->velocity : Math::Vec2{};
    const Math::Vec2 relativeVelocity =
        velocityBForContact - velocityAForContact;
    const float normalVelocity = relativeVelocity.Dot(normal);
    if (!IsFinite(positionA) || !IsFinite(positionB) ||
        !std::isfinite(normalVelocity)) {
        return;
    }

    Math::Vec2 velocityA = a.body->velocity;
    Math::Vec2 velocityB = b.body->velocity;
    if (normalVelocity < 0.0f) {
        const float impulseMagnitude = -normalVelocity / inverseMassSum;
        if (a.body->inverseMass > 0.0f) {
            velocityA -= normal * (impulseMagnitude * a.body->inverseMass);
        }
        if (b.body->inverseMass > 0.0f) {
            velocityB += normal * (impulseMagnitude * b.body->inverseMass);
        }
    }
    if (!IsFinite(velocityA) || !IsFinite(velocityB)) return;

    a.transform->position = positionA;
    b.transform->position = positionB;
    a.body->velocity = velocityA;
    b.body->velocity = velocityB;
}

void ResolveContacts(ECS::World& world) {
    std::vector<CollisionBody> bodies;
    world.ForEach<Transform2D, RigidBody2D, BoxCollider2D>(
        [&bodies](ECS::Entity entity, Transform2D& transform,
                  RigidBody2D& body, BoxCollider2D& collider) {
            if (IsValidCollisionBody(transform, body, collider)) {
                bodies.push_back({entity, &transform, &body, &collider});
            }
        });

    std::sort(bodies.begin(), bodies.end(),
              [](const CollisionBody& a, const CollisionBody& b) {
                  return a.entity.id < b.entity.id;
              });
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        for (std::size_t j = i + 1; j < bodies.size(); ++j) {
            ResolvePair(bodies[i], bodies[j]);
        }
    }
}

} // namespace

bool RigidBody2D::ApplyForce(const Math::Vec2& force) {
    if (!IsFinite(force)) return false;
    const Math::Vec2 accumulated = accumulatedForce + force;
    if (!IsFinite(accumulated)) return false;
    accumulatedForce = accumulated;
    return true;
}

void RegisterPhysicsComponents(ECS::World& world) {
    world.RegisterComponent<Transform2D>("PhysicsTransform2D");
    world.RegisterComponent<RigidBody2D>("PhysicsRigidBody2D");
    world.RegisterComponent<BoxCollider2D>("PhysicsBoxCollider2D");
}

Physics2DSystem::Physics2DSystem(Math::Vec2 gravity) : m_Gravity(gravity) {}

void Physics2DSystem::Initialize(ECS::World* world) {
    if (world) RegisterPhysicsComponents(*world);
}

void Physics2DSystem::Update(ECS::World* world, float deltaTime) {
    if (!world || !std::isfinite(deltaTime) || deltaTime <= 0.0f ||
        !IsFinite(m_Gravity)) {
        return;
    }

    world->ForEach<Transform2D, RigidBody2D>(
        [this, deltaTime](ECS::Entity, Transform2D& transform,
                          RigidBody2D& body) {
            if (!std::isfinite(body.inverseMass) || body.inverseMass < 0.0f ||
                !IsFinite(transform.position) || !IsFinite(body.velocity) ||
                !IsFinite(body.accumulatedForce)) {
                return;
            }

            if (body.inverseMass == 0.0f) {
                body.accumulatedForce = {};
                return;
            }

            const Math::Vec2 acceleration =
                m_Gravity + body.accumulatedForce * body.inverseMass;
            const Math::Vec2 velocity =
                body.velocity + acceleration * deltaTime;
            const Math::Vec2 position =
                transform.position + velocity * deltaTime;
            if (!IsFinite(acceleration) || !IsFinite(velocity) ||
                !IsFinite(position)) {
                return;
            }

            body.velocity = velocity;
            transform.position = position;
            body.accumulatedForce = {};
        });

    ResolveContacts(*world);
}

void Physics2DSystem::DescribeAccess(ECS::AccessSpec& access) const {
    access.Write<Transform2D>();
    access.Write<RigidBody2D>();
    access.Read<BoxCollider2D>();
}

} // namespace StonePotatoes::Physics
