#include "Physics2D.h"

#include "ecs/World.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <map>
#include <utility>
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

struct CellCoordinate {
    std::int64_t x;
    std::int64_t y;

    bool operator<(const CellCoordinate& other) const {
        return x < other.x || (x == other.x && y < other.y);
    }
};

struct CellRange {
    std::int64_t minX;
    std::int64_t maxX;
    std::int64_t minY;
    std::int64_t maxY;
};

constexpr double kBroadPhaseCellSize = 2.0;
constexpr double kMaxCellsPerCollider = 256.0;
constexpr float kContactPositionSlop = 0.001f;
constexpr std::size_t kMaxContactIterations = 16;

bool IsValidCollisionBody(const Transform2D& transform,
                          const RigidBody2D& body,
                          const BoxCollider2D& collider) {
    return IsFinite(transform.position) && IsFinite(body.velocity) &&
           IsFinite(body.accumulatedForce) &&
           std::isfinite(body.inverseMass) && body.inverseMass >= 0.0f &&
           IsFinite(collider.halfExtents) && collider.halfExtents.x > 0.0f &&
           collider.halfExtents.y > 0.0f;
}

bool ToCellCoordinate(double value, std::int64_t& result) {
    const double minimum = -std::ldexp(1.0, 63);
    const double maximumExclusive = std::ldexp(1.0, 63);
    if (!std::isfinite(value) || value < minimum ||
        value >= maximumExclusive) {
        return false;
    }
    result = static_cast<std::int64_t>(value);
    return true;
}

bool GetCellRange(const CollisionBody& body, CellRange& range) {
    const double x = static_cast<double>(body.transform->position.x);
    const double y = static_cast<double>(body.transform->position.y);
    const double halfWidth =
        static_cast<double>(body.collider->halfExtents.x);
    const double halfHeight =
        static_cast<double>(body.collider->halfExtents.y);
    const double minCellX = std::floor((x - halfWidth) / kBroadPhaseCellSize);
    const double maxCellX = std::floor((x + halfWidth) / kBroadPhaseCellSize);
    const double minCellY = std::floor((y - halfHeight) / kBroadPhaseCellSize);
    const double maxCellY = std::floor((y + halfHeight) / kBroadPhaseCellSize);
    return ToCellCoordinate(minCellX, range.minX) &&
           ToCellCoordinate(maxCellX, range.maxX) &&
           ToCellCoordinate(minCellY, range.minY) &&
           ToCellCoordinate(maxCellY, range.maxY);
}

bool IsSmallCellRange(const CellRange& range) {
    const double width = static_cast<double>(range.maxX) -
                         static_cast<double>(range.minX) + 1.0;
    const double height = static_cast<double>(range.maxY) -
                          static_cast<double>(range.minY) + 1.0;
    return width > 0.0 && height > 0.0 &&
           width <= kMaxCellsPerCollider &&
           height <= kMaxCellsPerCollider &&
           width * height <= kMaxCellsPerCollider;
}

bool ResolvePair(CollisionBody& a, CollisionBody& b) {
    const float dx = b.transform->position.x - a.transform->position.x;
    const float dy = b.transform->position.y - a.transform->position.y;
    const float combinedHalfWidth = a.collider->halfExtents.x +
                                    b.collider->halfExtents.x;
    const float combinedHalfHeight = a.collider->halfExtents.y +
                                     b.collider->halfExtents.y;
    if (!std::isfinite(dx) || !std::isfinite(dy) ||
        !std::isfinite(combinedHalfWidth) ||
        !std::isfinite(combinedHalfHeight)) {
        return false;
    }

    const float overlapX = combinedHalfWidth - std::fabs(dx);
    const float overlapY = combinedHalfHeight - std::fabs(dy);
    if (overlapX <= 0.0f || overlapY <= 0.0f) return false;

    // Resolve the axis with the least penetration. Ties use X, and coincident
    // centers point from the lower entity ID toward the higher entity ID.
    const bool useX = overlapX <= overlapY;
    const float penetration = useX ? overlapX : overlapY;
    const float signedDelta = useX ? dx : dy;
    const Math::Vec2 normal = useX
        ? Math::Vec2{signedDelta < 0.0f ? -1.0f : 1.0f, 0.0f}
        : Math::Vec2{0.0f, signedDelta < 0.0f ? -1.0f : 1.0f};

    const float inverseMassSum = a.body->inverseMass + b.body->inverseMass;
    if (!std::isfinite(inverseMassSum) || inverseMassSum <= 0.0f)
        return false;

    const float aShare = a.body->inverseMass / inverseMassSum;
    const float bShare = b.body->inverseMass / inverseMassSum;
    const float correctionDepth =
        penetration > kContactPositionSlop ? penetration : 0.0f;
    const Math::Vec2 correctionA = normal * (-correctionDepth * aShare);
    const Math::Vec2 correctionB = normal * (correctionDepth * bShare);
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
        return false;
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
    if (!IsFinite(velocityA) || !IsFinite(velocityB)) return false;

    const bool changed = positionA.x != a.transform->position.x ||
                         positionA.y != a.transform->position.y ||
                         positionB.x != b.transform->position.x ||
                         positionB.y != b.transform->position.y ||
                         velocityA.x != a.body->velocity.x ||
                         velocityA.y != a.body->velocity.y ||
                         velocityB.x != b.body->velocity.x ||
                         velocityB.y != b.body->velocity.y;
    if (!changed) return false;

    a.transform->position = positionA;
    b.transform->position = positionB;
    a.body->velocity = velocityA;
    b.body->velocity = velocityB;
    return true;
}

std::vector<std::pair<std::size_t, std::size_t>>
BuildCandidates(const std::vector<CollisionBody>& bodies) {
    // The ordered map makes cell traversal stable. Very large colliders (or
    // coordinates outside the integer grid range) use a conservative fallback
    // that pairs them with every body rather than allocating huge cell ranges.
    std::map<CellCoordinate, std::vector<std::size_t>> grid;
    std::vector<std::size_t> oversized;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        CellRange range{};
        if (!GetCellRange(bodies[i], range) || !IsSmallCellRange(range)) {
            oversized.push_back(i);
            continue;
        }

        for (std::int64_t x = range.minX;; ++x) {
            for (std::int64_t y = range.minY;; ++y) {
                grid[{x, y}].push_back(i);
                if (y == range.maxY) break;
            }
            if (x == range.maxX) break;
        }
    }

    std::vector<std::pair<std::size_t, std::size_t>> candidates;
    for (const auto& cell : grid) {
        const auto& members = cell.second;
        for (std::size_t i = 0; i < members.size(); ++i) {
            for (std::size_t j = i + 1; j < members.size(); ++j) {
                candidates.emplace_back(members[i], members[j]);
            }
        }
    }
    for (std::size_t large : oversized) {
        for (std::size_t other = 0; other < bodies.size(); ++other) {
            if (large == other) continue;
            candidates.emplace_back(std::min(large, other),
                                    std::max(large, other));
        }
    }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()),
                     candidates.end());
    return candidates;
}

struct ContactSolveStats {
    std::size_t uniqueCandidatePairs = 0;
    std::size_t iterations = 0;
    bool converged = true;
};

ContactSolveStats ResolveContacts(ECS::World& world) {
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

    ContactSolveStats stats;
    std::vector<std::pair<std::size_t, std::size_t>> allCandidates;
    for (std::size_t iteration = 0;
         iteration < kMaxContactIterations; ++iteration) {
        const auto candidates = BuildCandidates(bodies);
        allCandidates.insert(allCandidates.end(), candidates.begin(),
                             candidates.end());

        bool changed = false;
        for (const auto& candidate : candidates) {
            if (ResolvePair(bodies[candidate.first],
                            bodies[candidate.second])) {
                changed = true;
            }
        }
        stats.iterations = iteration + 1;
        if (!changed) {
            stats.converged = true;
            break;
        }
        stats.converged = false;
    }

    std::sort(allCandidates.begin(), allCandidates.end());
    allCandidates.erase(std::unique(allCandidates.begin(), allCandidates.end()),
                        allCandidates.end());
    stats.uniqueCandidatePairs = allCandidates.size();
    return stats;
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
    m_LastBroadPhasePairCount = 0;
    m_LastContactIterationCount = 0;
    m_LastContactSolveConverged = true;
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

    const ContactSolveStats stats = ResolveContacts(*world);
    m_LastBroadPhasePairCount = stats.uniqueCandidatePairs;
    m_LastContactIterationCount = stats.iterations;
    m_LastContactSolveConverged = stats.converged;
}

void Physics2DSystem::DescribeAccess(ECS::AccessSpec& access) const {
    access.Write<Transform2D>();
    access.Write<RigidBody2D>();
    access.Read<BoxCollider2D>();
}

} // namespace StonePotatoes::Physics
