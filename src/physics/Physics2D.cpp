#include "Physics2D.h"

#include "ecs/World.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <limits>
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
    float restitution;
    float rotationRadians;
    Math::Vec2 previousPosition;
};

Math::Vec2 GetAabbHalfExtents(const CollisionBody& body) {
    const float cosine = std::fabs(std::cos(body.rotationRadians));
    const float sine = std::fabs(std::sin(body.rotationRadians));
    return {cosine * body.collider->halfExtents.x +
                sine * body.collider->halfExtents.y,
            sine * body.collider->halfExtents.x +
                cosine * body.collider->halfExtents.y};
}

void GetBoxAxes(float radians, Math::Vec2& xAxis, Math::Vec2& yAxis) {
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    xAxis = {cosine, sine};
    yAxis = {-sine, cosine};
}

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
                          const BoxCollider2D& collider,
                          float restitution, float rotationRadians) {
    return IsFinite(transform.position) && IsFinite(body.velocity) &&
           IsFinite(body.accumulatedForce) &&
           std::isfinite(rotationRadians) &&
           std::isfinite(body.inverseMass) && body.inverseMass >= 0.0f &&
           IsFinite(collider.halfExtents) && collider.halfExtents.x > 0.0f &&
           collider.halfExtents.y > 0.0f &&
           std::isfinite(collider.staticFriction) &&
           std::isfinite(collider.dynamicFriction) &&
           collider.staticFriction >= 0.0f &&
           collider.dynamicFriction >= 0.0f &&
           collider.dynamicFriction <= collider.staticFriction &&
           std::isfinite(restitution) && restitution >= 0.0f &&
           restitution <= 1.0f;
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
    const Math::Vec2 aabbHalfExtents = GetAabbHalfExtents(body);
    const double halfWidth = static_cast<double>(aabbHalfExtents.x);
    const double halfHeight = static_cast<double>(aabbHalfExtents.y);
    const double minCellX = std::floor((x - halfWidth) / kBroadPhaseCellSize);
    const double maxCellX = std::floor((x + halfWidth) / kBroadPhaseCellSize);
    const double minCellY = std::floor((y - halfHeight) / kBroadPhaseCellSize);
    const double maxCellY = std::floor((y + halfHeight) / kBroadPhaseCellSize);
    return ToCellCoordinate(minCellX, range.minX) &&
           ToCellCoordinate(maxCellX, range.maxX) &&
           ToCellCoordinate(minCellY, range.minY) &&
           ToCellCoordinate(maxCellY, range.maxY);
}

bool GetSweptCellRange(const CollisionBody& body, CellRange& range) {
    const double startX = static_cast<double>(body.previousPosition.x);
    const double startY = static_cast<double>(body.previousPosition.y);
    const double endX = static_cast<double>(body.transform->position.x);
    const double endY = static_cast<double>(body.transform->position.y);
    const Math::Vec2 aabbHalfExtents = GetAabbHalfExtents(body);
    const double halfWidth = static_cast<double>(aabbHalfExtents.x);
    const double halfHeight = static_cast<double>(aabbHalfExtents.y);
    const double minCellX = std::floor(
        (std::min(startX, endX) - halfWidth) / kBroadPhaseCellSize);
    const double maxCellX = std::floor(
        (std::max(startX, endX) + halfWidth) / kBroadPhaseCellSize);
    const double minCellY = std::floor(
        (std::min(startY, endY) - halfHeight) / kBroadPhaseCellSize);
    const double maxCellY = std::floor(
        (std::max(startY, endY) + halfHeight) / kBroadPhaseCellSize);
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

bool CalculateContactVelocities(const CollisionBody& a,
                                const CollisionBody& b,
                                const Math::Vec2& normal,
                                Math::Vec2& velocityA,
                                Math::Vec2& velocityB) {
    const float inverseMassSum = a.body->inverseMass + b.body->inverseMass;
    if (!std::isfinite(inverseMassSum) || inverseMassSum <= 0.0f)
        return false;

    const Math::Vec2 velocityAForContact = a.body->inverseMass > 0.0f
        ? a.body->velocity : Math::Vec2{};
    const Math::Vec2 velocityBForContact = b.body->inverseMass > 0.0f
        ? b.body->velocity : Math::Vec2{};
    const Math::Vec2 relativeVelocity =
        velocityBForContact - velocityAForContact;
    const float normalVelocity = relativeVelocity.Dot(normal);
    if (!std::isfinite(normalVelocity)) return false;

    velocityA = a.body->velocity;
    velocityB = b.body->velocity;
    float normalImpulse = 0.0f;
    if (normalVelocity < 0.0f) {
        const float restitution = std::max(a.restitution, b.restitution);
        normalImpulse = -(1.0f + restitution) * normalVelocity /
                        inverseMassSum;
        if (!std::isfinite(normalImpulse)) return false;
        if (a.body->inverseMass > 0.0f) {
            velocityA -= normal * (normalImpulse * a.body->inverseMass);
        }
        if (b.body->inverseMass > 0.0f) {
            velocityB += normal * (normalImpulse * b.body->inverseMass);
        }
    }

    if (normalImpulse > 0.0f) {
        const Math::Vec2 tangent{-normal.y, normal.x};
        const Math::Vec2 velocityAForFriction = a.body->inverseMass > 0.0f
            ? velocityA : Math::Vec2{};
        const Math::Vec2 velocityBForFriction = b.body->inverseMass > 0.0f
            ? velocityB : Math::Vec2{};
        const float tangentVelocity =
            (velocityBForFriction - velocityAForFriction).Dot(tangent);
        if (!std::isfinite(tangentVelocity)) return false;

        const float desiredTangentImpulse =
            -tangentVelocity / inverseMassSum;
        const double staticFriction = std::sqrt(
            static_cast<double>(a.collider->staticFriction) *
            static_cast<double>(b.collider->staticFriction));
        const double dynamicFriction = std::sqrt(
            static_cast<double>(a.collider->dynamicFriction) *
            static_cast<double>(b.collider->dynamicFriction));
        const double staticLimit =
            staticFriction * static_cast<double>(normalImpulse);
        const double desiredMagnitude =
            std::fabs(static_cast<double>(desiredTangentImpulse));
        const double tangentImpulseValue =
            desiredMagnitude <= staticLimit
                ? static_cast<double>(desiredTangentImpulse)
                : -std::copysign(
                      dynamicFriction * static_cast<double>(normalImpulse),
                      static_cast<double>(tangentVelocity));
        if (!std::isfinite(desiredTangentImpulse) ||
            !std::isfinite(tangentImpulseValue) ||
            std::fabs(tangentImpulseValue) >
                static_cast<double>(std::numeric_limits<float>::max())) {
            return false;
        }

        const float tangentImpulse = static_cast<float>(tangentImpulseValue);
        if (a.body->inverseMass > 0.0f) {
            velocityA -=
                tangent * (tangentImpulse * a.body->inverseMass);
        }
        if (b.body->inverseMass > 0.0f) {
            velocityB +=
                tangent * (tangentImpulse * b.body->inverseMass);
        }
    }

    return IsFinite(velocityA) && IsFinite(velocityB);
}

bool ResolvePair(CollisionBody& a, CollisionBody& b) {
    const Math::Vec2 centerDelta =
        b.transform->position - a.transform->position;
    if (!IsFinite(centerDelta)) return false;

    Math::Vec2 axesA[2];
    Math::Vec2 axesB[2];
    GetBoxAxes(a.rotationRadians, axesA[0], axesA[1]);
    GetBoxAxes(b.rotationRadians, axesB[0], axesB[1]);
    const Math::Vec2 candidateAxes[] = {
        axesA[0], axesA[1], axesB[0], axesB[1]};

    float penetration = std::numeric_limits<float>::infinity();
    Math::Vec2 normal{};
    for (const Math::Vec2& axis : candidateAxes) {
        const float radiusA =
            a.collider->halfExtents.x * std::fabs(axesA[0].Dot(axis)) +
            a.collider->halfExtents.y * std::fabs(axesA[1].Dot(axis));
        const float radiusB =
            b.collider->halfExtents.x * std::fabs(axesB[0].Dot(axis)) +
            b.collider->halfExtents.y * std::fabs(axesB[1].Dot(axis));
        const float signedDistance = centerDelta.Dot(axis);
        const float overlap = radiusA + radiusB - std::fabs(signedDistance);
        if (!std::isfinite(radiusA) || !std::isfinite(radiusB) ||
            !std::isfinite(signedDistance) || !std::isfinite(overlap)) {
            return false;
        }
        if (overlap <= 0.0f) return false;
        if (overlap < penetration) {
            penetration = overlap;
            normal = axis * (signedDistance < 0.0f ? -1.0f : 1.0f);
        }
    }

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

    if (!IsFinite(positionA) || !IsFinite(positionB)) return false;
    Math::Vec2 velocityA{};
    Math::Vec2 velocityB{};
    if (!CalculateContactVelocities(a, b, normal, velocityA, velocityB))
        return false;

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
BuildCandidates(const std::vector<CollisionBody>& bodies,
                bool includeSweptBounds = false) {
    // The ordered map makes cell traversal stable. Very large colliders (or
    // coordinates outside the integer grid range) use a conservative fallback
    // that pairs them with every body rather than allocating huge cell ranges.
    std::map<CellCoordinate, std::vector<std::size_t>> grid;
    std::vector<std::size_t> oversized;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        CellRange range{};
        const bool hasRange = includeSweptBounds
            ? GetSweptCellRange(bodies[i], range)
            : GetCellRange(bodies[i], range);
        if (!hasRange || !IsSmallCellRange(range)) {
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

struct SweepHit {
    float fraction = 0.0f;
    Math::Vec2 normal{};
};

bool FindSweptBoxHit(const CollisionBody& a, const CollisionBody& b,
                     const Math::Vec2& startA, const Math::Vec2& endA,
                     const Math::Vec2& startB, const Math::Vec2& endB,
                     SweepHit& hit) {
    const Math::Vec2 relativeStart = startB - startA;
    const Math::Vec2 relativeDisplacement = (endB - startB) - (endA - startA);
    if (!IsFinite(relativeStart) || !IsFinite(relativeDisplacement))
        return false;

    Math::Vec2 axesA[2];
    Math::Vec2 axesB[2];
    GetBoxAxes(a.rotationRadians, axesA[0], axesA[1]);
    GetBoxAxes(b.rotationRadians, axesB[0], axesB[1]);
    const Math::Vec2 axes[] = {axesA[0], axesA[1], axesB[0], axesB[1]};

    double entry = -std::numeric_limits<double>::infinity();
    double exit = std::numeric_limits<double>::infinity();
    double minimumInitialPenetration =
        std::numeric_limits<double>::infinity();
    Math::Vec2 initialNormal{};
    Math::Vec2 entryNormal{};
    double entryMotion = 0.0;
    constexpr double kContactTolerance = 1.0e-5;

    for (const Math::Vec2& axis : axes) {
        const double radiusA =
            static_cast<double>(a.collider->halfExtents.x) *
                std::fabs(static_cast<double>(axesA[0].Dot(axis))) +
            static_cast<double>(a.collider->halfExtents.y) *
                std::fabs(static_cast<double>(axesA[1].Dot(axis)));
        const double radiusB =
            static_cast<double>(b.collider->halfExtents.x) *
                std::fabs(static_cast<double>(axesB[0].Dot(axis))) +
            static_cast<double>(b.collider->halfExtents.y) *
                std::fabs(static_cast<double>(axesB[1].Dot(axis)));
        const double extent = radiusA + radiusB;
        const double origin =
            static_cast<double>(relativeStart.x) * axis.x +
            static_cast<double>(relativeStart.y) * axis.y;
        const double displacement =
            static_cast<double>(relativeDisplacement.x) * axis.x +
            static_cast<double>(relativeDisplacement.y) * axis.y;
        const double penetration = extent - std::fabs(origin);
        if (!std::isfinite(extent) || !std::isfinite(origin) ||
            !std::isfinite(displacement) || !std::isfinite(penetration)) {
            return false;
        }
        if (penetration < minimumInitialPenetration) {
            minimumInitialPenetration = penetration;
            initialNormal = axis * (origin < 0.0 ? -1.0f : 1.0f);
        }

        if (displacement == 0.0) {
            if (std::fabs(origin) > extent) return false;
            continue;
        }

        const double first = (-extent - origin) / displacement;
        const double second = (extent - origin) / displacement;
        const double axisEntry = std::min(first, second);
        const double axisExit = std::max(first, second);
        if (axisEntry > entry) {
            entry = axisEntry;
            entryMotion = displacement;
            entryNormal = axis * (origin + displacement * entry < 0.0
                                      ? -1.0f : 1.0f);
        }
        exit = std::min(exit, axisExit);
        if (entry > exit) return false;
    }

    // Existing/tiny event-time overlap is handled at t=0 only when closing;
    // deeper initial overlaps remain the discrete solver's responsibility.
    if (minimumInitialPenetration >= -kContactTolerance &&
        minimumInitialPenetration <= kContactTolerance) {
        const float closingSpeed = relativeDisplacement.Dot(initialNormal);
        if (closingSpeed < 0.0f) {
            hit.fraction = 0.0f;
            hit.normal = initialNormal;
            return true;
        }
        return false;
    }
    if (minimumInitialPenetration > kContactTolerance) return false;
    if (!std::isfinite(entry) || !std::isfinite(exit) || entry > exit ||
        exit < 0.0 || entry < 0.0 || entry > 1.0 ||
        entryMotion == 0.0) {
        return false;
    }

    hit.fraction = static_cast<float>(entry);
    hit.normal = entryNormal;
    return true;
}

void ResolveContinuousContacts(
    std::vector<CollisionBody>& bodies,
    float deltaTime,
    std::vector<std::pair<std::size_t, std::size_t>>& allCandidates) {
    const std::size_t maxEvents = bodies.size() >= 64
        ? 256 : std::max<std::size_t>(4, bodies.size() * 4);
    std::vector<Math::Vec2> segmentStarts(bodies.size());
    std::vector<Math::Vec2> motionVelocities(bodies.size());
    bool hasMotion = false;

    for (std::size_t i = 0; i < bodies.size(); ++i) {
        CollisionBody& body = bodies[i];
        segmentStarts[i] = body.body->inverseMass > 0.0f
            ? body.previousPosition : body.transform->position;
        if (body.body->inverseMass > 0.0f &&
            (segmentStarts[i].x != body.transform->position.x ||
             segmentStarts[i].y != body.transform->position.y)) {
            motionVelocities[i] = body.body->velocity;
            hasMotion = true;
        }
        body.previousPosition = segmentStarts[i];
        body.transform->position = segmentStarts[i];
    }

    if (!hasMotion || deltaTime <= 0.0f) return;

    float remainingTime = deltaTime;
    for (std::size_t event = 0; event < maxEvents && remainingTime > 0.0f;
         ++event) {
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            CollisionBody& body = bodies[i];
            body.previousPosition = segmentStarts[i];
            body.transform->position = body.body->inverseMass > 0.0f
                ? segmentStarts[i] + motionVelocities[i] * remainingTime
                : segmentStarts[i];
        }

        const auto candidates = BuildCandidates(bodies, true);
        allCandidates.insert(allCandidates.end(), candidates.begin(),
                             candidates.end());

        bool foundHit = false;
        float earliestFraction = std::numeric_limits<float>::infinity();
        std::size_t firstIndex = 0;
        std::size_t secondIndex = 0;
        SweepHit earliestHit{};
        for (const auto& candidate : candidates) {
            const CollisionBody& a = bodies[candidate.first];
            const CollisionBody& b = bodies[candidate.second];
            if (a.body->inverseMass + b.body->inverseMass <= 0.0f) continue;
            SweepHit hit;
            if (FindSweptBoxHit(a, b, segmentStarts[candidate.first],
                                a.transform->position,
                                segmentStarts[candidate.second],
                                b.transform->position, hit) &&
                (!foundHit || hit.fraction < earliestFraction)) {
                foundHit = true;
                earliestFraction = hit.fraction;
                firstIndex = candidate.first;
                secondIndex = candidate.second;
                earliestHit = hit;
            }
        }

        if (!foundHit) return;

        const float elapsedTime = remainingTime * earliestHit.fraction;
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i].body->inverseMass > 0.0f) {
                segmentStarts[i] += motionVelocities[i] * elapsedTime;
                bodies[i].transform->position = segmentStarts[i];
            }
        }

        Math::Vec2 velocityA{};
        Math::Vec2 velocityB{};
        CollisionBody& a = bodies[firstIndex];
        CollisionBody& b = bodies[secondIndex];
        if (!CalculateContactVelocities(a, b, earliestHit.normal,
                                        velocityA, velocityB)) {
            // Keep bodies at the last safe time of impact if the impulse would
            // produce a non-finite state.
            return;
        }

        const float nextRemainingTime =
            remainingTime * (1.0f - earliestHit.fraction);
        if (!std::isfinite(nextRemainingTime)) return;
        a.body->velocity = velocityA;
        b.body->velocity = velocityB;
        if (a.body->inverseMass > 0.0f) motionVelocities[firstIndex] = velocityA;
        if (b.body->inverseMass > 0.0f) motionVelocities[secondIndex] = velocityB;
        remainingTime = nextRemainingTime;
    }

    // If the event budget is exhausted, positions remain at the last resolved
    // impact, conservatively discarding the unprocessed part of the tick.
}

struct ContactSolveStats {
    std::size_t uniqueCandidatePairs = 0;
    std::size_t iterations = 0;
    bool converged = true;
};

ContactSolveStats ResolveContacts(
    ECS::World& world,
    const std::map<ECS::EntityID, Math::Vec2>& previousPositions,
    float deltaTime) {
    std::vector<CollisionBody> bodies;
    world.ForEach<Transform2D, RigidBody2D, BoxCollider2D>(
        [&world, &bodies, &previousPositions](ECS::Entity entity,
                                              Transform2D& transform,
                                              RigidBody2D& body,
                                              BoxCollider2D& collider) {
            const Rotation2D* rotation =
                world.GetComponent<Rotation2D>(entity);
            const float rotationRadians = rotation ? rotation->radians : 0.0f;
            const Restitution2D* material =
                world.GetComponent<Restitution2D>(entity);
            const float restitution = material ? material->coefficient : 0.0f;
            if (IsValidCollisionBody(transform, body, collider,
                                     restitution, rotationRadians)) {
                const auto previous = previousPositions.find(entity.id);
                const Math::Vec2 previousPosition =
                    previous == previousPositions.end()
                        ? transform.position : previous->second;
                bodies.push_back(
                    {entity, &transform, &body, &collider, restitution,
                     rotationRadians,
                     previousPosition});
            }
        });

    std::sort(bodies.begin(), bodies.end(),
              [](const CollisionBody& a, const CollisionBody& b) {
                  return a.entity.id < b.entity.id;
              });

    ContactSolveStats stats;
    std::vector<std::pair<std::size_t, std::size_t>> allCandidates;
    const bool hasMovingPath = std::any_of(
        bodies.begin(), bodies.end(), [](const CollisionBody& body) {
            return body.body->inverseMass > 0.0f &&
                   (body.previousPosition.x != body.transform->position.x ||
                    body.previousPosition.y != body.transform->position.y);
        });
    if (bodies.size() > 1 && hasMovingPath) {
        ResolveContinuousContacts(bodies, deltaTime, allCandidates);
    }

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
    world.RegisterComponent<Rotation2D>("PhysicsRotation2D");
    world.RegisterComponent<RigidBody2D>("PhysicsRigidBody2D");
    world.RegisterComponent<BoxCollider2D>("PhysicsBoxCollider2D");
    world.RegisterComponent<Restitution2D>("PhysicsRestitution2D");
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

    std::map<ECS::EntityID, Math::Vec2> previousPositions;
    world->ForEach<Transform2D, RigidBody2D>(
        [this, deltaTime, &previousPositions](ECS::Entity entity,
                                              Transform2D& transform,
                                              RigidBody2D& body) {
            previousPositions[entity.id] = transform.position;
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

    const ContactSolveStats stats =
        ResolveContacts(*world, previousPositions, deltaTime);
    m_LastBroadPhasePairCount = stats.uniqueCandidatePairs;
    m_LastContactIterationCount = stats.iterations;
    m_LastContactSolveConverged = stats.converged;
}

void Physics2DSystem::DescribeAccess(ECS::AccessSpec& access) const {
    access.Write<Transform2D>();
    access.Write<RigidBody2D>();
    access.Read<BoxCollider2D>();
    access.Read<Rotation2D>();
    access.Read<Restitution2D>();
}

} // namespace StonePotatoes::Physics
