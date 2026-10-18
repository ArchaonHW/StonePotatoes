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
    Rotation2D* rotation;
    RigidBody2D* body;
    BoxCollider2D* collider;
    float restitution;
    float rotationRadians;
    Math::Vec2 previousPosition;
    float previousRotationRadians;
};

float GetRotationRate(const CollisionBody& body) {
    return body.rotation && body.body->inverseInertia > 0.0f
        ? body.body->angularVelocity : 0.0f;
}

float GetCollisionInverseInertia(const CollisionBody& body) {
    return body.rotation ? body.body->inverseInertia : 0.0f;
}

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
           std::isfinite(body.angularVelocity) &&
           std::isfinite(body.accumulatedTorque) &&
           std::isfinite(body.inverseInertia) && body.inverseInertia >= 0.0f &&
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
    Math::Vec2 aabbHalfExtents = GetAabbHalfExtents(body);
    if (GetRotationRate(body) != 0.0f) {
        const float radius = std::sqrt(
            body.collider->halfExtents.x * body.collider->halfExtents.x +
            body.collider->halfExtents.y * body.collider->halfExtents.y);
        aabbHalfExtents = {radius, radius};
    }
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

float Cross(const Math::Vec2& a, const Math::Vec2& b) {
    return a.x * b.y - a.y * b.x;
}

float ProjectionRadius(const CollisionBody& body, const Math::Vec2& axis) {
    Math::Vec2 axes[2];
    GetBoxAxes(body.rotationRadians, axes[0], axes[1]);
    return body.collider->halfExtents.x * std::fabs(axes[0].Dot(axis)) +
           body.collider->halfExtents.y * std::fabs(axes[1].Dot(axis));
}

float ProjectionRadius(const BoxCollider2D& collider, float rotationRadians,
                       const Math::Vec2& axis) {
    Math::Vec2 axes[2];
    GetBoxAxes(rotationRadians, axes[0], axes[1]);
    return collider.halfExtents.x * std::fabs(axes[0].Dot(axis)) +
           collider.halfExtents.y * std::fabs(axes[1].Dot(axis));
}

bool FindContactPoint(const CollisionBody& a, const CollisionBody& b,
                      const Math::Vec2& normal, Math::Vec2& contactPoint) {
    const Math::Vec2 tangent{-normal.y, normal.x};
    const float radiusNormalA = ProjectionRadius(a, normal);
    const float radiusNormalB = ProjectionRadius(b, normal);
    const float radiusTangentA = ProjectionRadius(a, tangent);
    const float radiusTangentB = ProjectionRadius(b, tangent);
    const float normalA = a.transform->position.Dot(normal) + radiusNormalA;
    const float normalB = b.transform->position.Dot(normal) - radiusNormalB;
    const float tangentCenterA = a.transform->position.Dot(tangent);
    const float tangentCenterB = b.transform->position.Dot(tangent);
    const float tangentMinimum = std::max(
        tangentCenterA - radiusTangentA, tangentCenterB - radiusTangentB);
    const float tangentMaximum = std::min(
        tangentCenterA + radiusTangentA, tangentCenterB + radiusTangentB);
    if (!std::isfinite(radiusNormalA) || !std::isfinite(radiusNormalB) ||
        !std::isfinite(radiusTangentA) || !std::isfinite(radiusTangentB) ||
        !std::isfinite(normalA) || !std::isfinite(normalB) ||
        !std::isfinite(tangentMinimum) || !std::isfinite(tangentMaximum) ||
        tangentMinimum > tangentMaximum) {
        return false;
    }
    const float normalCoordinate = 0.5f * (normalA + normalB);
    const float tangentCoordinate = 0.5f * (tangentMinimum + tangentMaximum);
    contactPoint = normal * normalCoordinate + tangent * tangentCoordinate;
    return IsFinite(contactPoint);
}

struct SweepHit {
    float fraction = 0.0f;
    Math::Vec2 normal{};
};

bool FindSweptRotatingBoxHit(const CollisionBody& a, const CollisionBody& b,
                             const Math::Vec2& startA,
                             const Math::Vec2& velocityA,
                             float angleA,
                             const Math::Vec2& startB,
                             const Math::Vec2& velocityB,
                             float angleB,
                             float deltaTime,
                             SweepHit& hit) {
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0f) return false;
    const float angularRateA = GetRotationRate(a);
    const float angularRateB = GetRotationRate(b);
    const float radiusA = std::sqrt(
        a.collider->halfExtents.x * a.collider->halfExtents.x +
        a.collider->halfExtents.y * a.collider->halfExtents.y);
    const float radiusB = std::sqrt(
        b.collider->halfExtents.x * b.collider->halfExtents.x +
        b.collider->halfExtents.y * b.collider->halfExtents.y);
    const Math::Vec2 relativeVelocity = velocityB - velocityA;
    const float linearSpeed = std::sqrt(relativeVelocity.Dot(relativeVelocity));
    constexpr float kSeparationTolerance = 1.0e-4f;
    constexpr int kMaxAdvances = 128;

    float time = 0.0f;
    for (int advanceIndex = 0; advanceIndex < kMaxAdvances; ++advanceIndex) {
        const Math::Vec2 positionA = startA + velocityA * time;
        const Math::Vec2 positionB = startB + velocityB * time;
        const float rotationA = angleA + angularRateA * time;
        const float rotationB = angleB + angularRateB * time;
        const Math::Vec2 centerDelta = positionB - positionA;
        if (!IsFinite(positionA) || !IsFinite(positionB) ||
            !std::isfinite(rotationA) || !std::isfinite(rotationB) ||
            !IsFinite(centerDelta)) {
            return false;
        }

        Math::Vec2 axesA[2];
        Math::Vec2 axesB[2];
        GetBoxAxes(rotationA, axesA[0], axesA[1]);
        GetBoxAxes(rotationB, axesB[0], axesB[1]);
        const Math::Vec2 axes[] = {
            axesA[0], axesA[1], axesB[0], axesB[1]};
        float separation = -std::numeric_limits<float>::infinity();
        Math::Vec2 normal{};
        for (const Math::Vec2& axis : axes) {
            const float projectedA = ProjectionRadius(
                *a.collider, rotationA, axis);
            const float projectedB = ProjectionRadius(
                *b.collider, rotationB, axis);
            const float signedDistance = centerDelta.Dot(axis);
            const float axisSeparation = std::fabs(signedDistance) -
                                         projectedA - projectedB;
            if (!std::isfinite(projectedA) || !std::isfinite(projectedB) ||
                !std::isfinite(signedDistance) ||
                !std::isfinite(axisSeparation)) {
                return false;
            }
            if (axisSeparation > separation) {
                separation = axisSeparation;
                normal = axis * (signedDistance < 0.0f ? -1.0f : 1.0f);
            }
        }

        if (time > 0.0f && separation <= kSeparationTolerance) {
            const Math::Vec2 tangent{-normal.y, normal.x};
            const float tangentRadiusA = ProjectionRadius(
                *a.collider, rotationA, tangent);
            const float tangentRadiusB = ProjectionRadius(
                *b.collider, rotationB, tangent);
            const float tangentCenterA = positionA.Dot(tangent);
            const float tangentCenterB = positionB.Dot(tangent);
            const float tangentMin = std::max(
                tangentCenterA - tangentRadiusA,
                tangentCenterB - tangentRadiusB);
            const float tangentMax = std::min(
                tangentCenterA + tangentRadiusA,
                tangentCenterB + tangentRadiusB);
            if (!std::isfinite(tangentMin) || !std::isfinite(tangentMax) ||
                tangentMin > tangentMax) {
                return false;
            }
            const float normalSurfaceA = positionA.Dot(normal) +
                ProjectionRadius(*a.collider, rotationA, normal);
            const float normalSurfaceB = positionB.Dot(normal) -
                ProjectionRadius(*b.collider, rotationB, normal);
            const float tangentCoordinate = 0.5f * (tangentMin + tangentMax);
            const float normalCoordinate =
                0.5f * (normalSurfaceA + normalSurfaceB);
            const Math::Vec2 contact =
                normal * normalCoordinate + tangent * tangentCoordinate;
            const Math::Vec2 armA = contact - positionA;
            const Math::Vec2 armB = contact - positionB;
            const Math::Vec2 pointVelocityA = velocityA + Math::Vec2{
                -angularRateA * armA.y, angularRateA * armA.x};
            const Math::Vec2 pointVelocityB = velocityB + Math::Vec2{
                -angularRateB * armB.y, angularRateB * armB.x};
            const float closingSpeed =
                (pointVelocityB - pointVelocityA).Dot(normal);
            if (!std::isfinite(closingSpeed)) return false;
            if (closingSpeed < 0.0f) {
                hit.fraction = time / deltaTime;
                hit.normal = normal;
                return true;
            }
            time += std::max(deltaTime / 2048.0f, 1.0e-6f);
            if (time >= deltaTime) return false;
            continue;
        }

        // Initial overlap recovery remains the discrete solver's job.
        if (time == 0.0f && separation <= 0.0f) return false;
        const float centerDistance = std::sqrt(centerDelta.Dot(centerDelta));
        const float angularSpeed =
            std::fabs(angularRateA) + std::fabs(angularRateB);
        const float speedBound = linearSpeed + angularSpeed *
            (centerDistance + radiusA + radiusB);
        if (!std::isfinite(centerDistance) || !std::isfinite(speedBound) ||
            speedBound <= 0.0f) {
            return false;
        }
        const float step = std::max(
            0.8f * std::max(separation, 0.0f) / speedBound,
            deltaTime * 1.0e-7f);
        if (!std::isfinite(step) || time + step > deltaTime) return false;
        time += step;
    }
    return false;
}

bool CalculateContactVelocities(const CollisionBody& a,
                                const CollisionBody& b,
                                const Math::Vec2& normal,
                                const Math::Vec2& contactPoint,
                                Math::Vec2& velocityA,
                                Math::Vec2& velocityB,
                                float& angularVelocityA,
                                float& angularVelocityB) {
    const Math::Vec2 armA = contactPoint - a.transform->position;
    const Math::Vec2 armB = contactPoint - b.transform->position;
    const float normalArmA = Cross(armA, normal);
    const float normalArmB = Cross(armB, normal);
    const float inverseMassA = a.body->inverseMass;
    const float inverseMassB = b.body->inverseMass;
    const float inverseInertiaA = GetCollisionInverseInertia(a);
    const float inverseInertiaB = GetCollisionInverseInertia(b);
    const float inverseNormalMass = inverseMassA + inverseMassB +
        inverseInertiaA * normalArmA * normalArmA +
        inverseInertiaB * normalArmB * normalArmB;
    if (!std::isfinite(inverseNormalMass) || inverseNormalMass <= 0.0f)
        return false;

    velocityA = a.body->velocity;
    velocityB = b.body->velocity;
    angularVelocityA = a.body->angularVelocity;
    angularVelocityB = b.body->angularVelocity;
    const auto pointVelocity = [](const Math::Vec2& linear, float angular,
                                  const Math::Vec2& arm) {
        return linear + Math::Vec2{-angular * arm.y, angular * arm.x};
    };
    const auto relativeAtContact = [&]() {
        const Math::Vec2 linearA = inverseMassA > 0.0f
            ? velocityA : Math::Vec2{};
        const Math::Vec2 linearB = inverseMassB > 0.0f
            ? velocityB : Math::Vec2{};
        const float angularA = inverseInertiaA > 0.0f
            ? angularVelocityA : 0.0f;
        const float angularB = inverseInertiaB > 0.0f
            ? angularVelocityB : 0.0f;
        const Math::Vec2 pointA = pointVelocity(linearA, angularA, armA);
        const Math::Vec2 pointB = pointVelocity(linearB, angularB, armB);
        return pointB - pointA;
    };

    const float normalVelocity = relativeAtContact().Dot(normal);
    if (!std::isfinite(normalVelocity)) return false;
    float normalImpulse = 0.0f;
    if (normalVelocity < 0.0f) {
        const float restitution = std::max(a.restitution, b.restitution);
        normalImpulse = -(1.0f + restitution) * normalVelocity /
                        inverseNormalMass;
        if (!std::isfinite(normalImpulse)) return false;
        if (inverseMassA > 0.0f)
            velocityA -= normal * (normalImpulse * inverseMassA);
        if (inverseMassB > 0.0f)
            velocityB += normal * (normalImpulse * inverseMassB);
        angularVelocityA -= normalImpulse * normalArmA * inverseInertiaA;
        angularVelocityB += normalImpulse * normalArmB * inverseInertiaB;
    }

    if (normalImpulse > 0.0f) {
        const Math::Vec2 tangent{-normal.y, normal.x};
        const float tangentArmA = Cross(armA, tangent);
        const float tangentArmB = Cross(armB, tangent);
        const float inverseTangentMass = inverseMassA + inverseMassB +
            inverseInertiaA * tangentArmA * tangentArmA +
            inverseInertiaB * tangentArmB * tangentArmB;
        const float tangentVelocity = relativeAtContact().Dot(tangent);
        if (!std::isfinite(inverseTangentMass) || inverseTangentMass <= 0.0f ||
            !std::isfinite(tangentVelocity)) {
            return false;
        }
        const float desiredTangentImpulse =
            -tangentVelocity / inverseTangentMass;
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
        if (inverseMassA > 0.0f)
            velocityA -= tangent * (tangentImpulse * inverseMassA);
        if (inverseMassB > 0.0f)
            velocityB += tangent * (tangentImpulse * inverseMassB);
        angularVelocityA -= tangentImpulse * tangentArmA * inverseInertiaA;
        angularVelocityB += tangentImpulse * tangentArmB * inverseInertiaB;
    }

    return IsFinite(velocityA) && IsFinite(velocityB) &&
           std::isfinite(angularVelocityA) &&
           std::isfinite(angularVelocityB);
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
    if (!std::isfinite(inverseMassSum) || inverseMassSum < 0.0f)
        return false;
    const float aShare = inverseMassSum > 0.0f
        ? a.body->inverseMass / inverseMassSum : 0.0f;
    const float bShare = inverseMassSum > 0.0f
        ? b.body->inverseMass / inverseMassSum : 0.0f;
    const float correctionDepth = inverseMassSum > 0.0f &&
                                  penetration > kContactPositionSlop
        ? penetration : 0.0f;
    const Math::Vec2 correctionA = normal * (-correctionDepth * aShare);
    const Math::Vec2 correctionB = normal * (correctionDepth * bShare);
    const Math::Vec2 positionA = a.transform->position + correctionA;
    const Math::Vec2 positionB = b.transform->position + correctionB;

    if (!IsFinite(positionA) || !IsFinite(positionB)) return false;
    Math::Vec2 contactPoint{};
    if (!FindContactPoint(a, b, normal, contactPoint)) return false;
    Math::Vec2 velocityA{};
    Math::Vec2 velocityB{};
    float angularVelocityA = 0.0f;
    float angularVelocityB = 0.0f;
    if (!CalculateContactVelocities(a, b, normal, contactPoint,
                                   velocityA, velocityB,
                                   angularVelocityA, angularVelocityB))
        return false;

    const bool changed = positionA.x != a.transform->position.x ||
                         positionA.y != a.transform->position.y ||
                         positionB.x != b.transform->position.x ||
                         positionB.y != b.transform->position.y ||
                         velocityA.x != a.body->velocity.x ||
                         velocityA.y != a.body->velocity.y ||
                         velocityB.x != b.body->velocity.x ||
                         velocityB.y != b.body->velocity.y ||
                         angularVelocityA != a.body->angularVelocity ||
                         angularVelocityB != b.body->angularVelocity;
    if (!changed) return false;

    a.transform->position = positionA;
    b.transform->position = positionB;
    a.body->velocity = velocityA;
    b.body->velocity = velocityB;
    a.body->angularVelocity = angularVelocityA;
    b.body->angularVelocity = angularVelocityB;
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
    std::vector<float> segmentStartAngles(bodies.size());
    bool hasMotion = false;

    for (std::size_t i = 0; i < bodies.size(); ++i) {
        CollisionBody& body = bodies[i];
        segmentStarts[i] = body.body->inverseMass > 0.0f
            ? body.previousPosition : body.transform->position;
        motionVelocities[i] = body.body->inverseMass > 0.0f
            ? body.body->velocity : Math::Vec2{};
        segmentStartAngles[i] = body.previousRotationRadians;
        if (body.rotation) {
            body.rotation->radians = segmentStartAngles[i];
            body.rotationRadians = segmentStartAngles[i];
        }
        if (motionVelocities[i].x != 0.0f ||
            motionVelocities[i].y != 0.0f ||
            (body.rotation && body.body->inverseInertia > 0.0f &&
             body.body->angularVelocity != 0.0f)) {
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
            body.rotationRadians = segmentStartAngles[i] +
                GetRotationRate(body) * remainingTime;
            if (body.rotation) body.rotation->radians = body.rotationRadians;
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
            if (a.body->inverseMass + a.body->inverseInertia +
                    b.body->inverseMass + b.body->inverseInertia <= 0.0f) {
                continue;
            }
            SweepHit hit;
            const float angularRateA = GetRotationRate(a);
            const float angularRateB = GetRotationRate(b);
            const bool hitFound = angularRateA != 0.0f || angularRateB != 0.0f
                ? FindSweptRotatingBoxHit(
                      a, b, segmentStarts[candidate.first],
                      motionVelocities[candidate.first],
                      segmentStartAngles[candidate.first],
                      segmentStarts[candidate.second],
                      motionVelocities[candidate.second],
                      segmentStartAngles[candidate.second], remainingTime, hit)
                : FindSweptBoxHit(a, b, segmentStarts[candidate.first],
                                  a.transform->position,
                                  segmentStarts[candidate.second],
                                  b.transform->position, hit);
            if (hitFound &&
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
            if (GetRotationRate(bodies[i]) != 0.0f) {
                segmentStartAngles[i] +=
                    GetRotationRate(bodies[i]) * elapsedTime;
                bodies[i].rotationRadians = segmentStartAngles[i];
                if (bodies[i].rotation)
                    bodies[i].rotation->radians = segmentStartAngles[i];
            }
        }

        Math::Vec2 velocityA{};
        Math::Vec2 velocityB{};
        float angularVelocityA = 0.0f;
        float angularVelocityB = 0.0f;
        CollisionBody& a = bodies[firstIndex];
        CollisionBody& b = bodies[secondIndex];
        Math::Vec2 contactPoint{};
        if (!FindContactPoint(a, b, earliestHit.normal, contactPoint) ||
            !CalculateContactVelocities(a, b, earliestHit.normal,
                                        contactPoint, velocityA, velocityB,
                                        angularVelocityA,
                                        angularVelocityB)) {
            // Keep bodies at the last safe time of impact if the impulse would
            // produce a non-finite state.
            return;
        }

        const float nextRemainingTime =
            remainingTime * (1.0f - earliestHit.fraction);
        if (!std::isfinite(nextRemainingTime)) return;
        a.body->velocity = velocityA;
        b.body->velocity = velocityB;
        a.body->angularVelocity = angularVelocityA;
        b.body->angularVelocity = angularVelocityB;
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
    const std::map<ECS::EntityID, float>& previousRotations,
    float deltaTime) {
    std::vector<CollisionBody> bodies;
    world.ForEach<Transform2D, RigidBody2D, BoxCollider2D>(
        [&world, &bodies, &previousPositions, &previousRotations](ECS::Entity entity,
                                              Transform2D& transform,
                                              RigidBody2D& body,
                                              BoxCollider2D& collider) {
            Rotation2D* rotation =
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
                const auto previousRotation =
                    previousRotations.find(entity.id);
                const float previousRotationRadians =
                    previousRotation == previousRotations.end()
                        ? rotationRadians : previousRotation->second;
                bodies.push_back({entity, &transform, rotation, &body,
                                  &collider, restitution, rotationRadians,
                                  previousPosition, previousRotationRadians});
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
            return (body.body->inverseMass > 0.0f &&
                    (body.previousPosition.x != body.transform->position.x ||
                     body.previousPosition.y != body.transform->position.y)) ||
                   (body.rotation && body.body->inverseInertia > 0.0f &&
                    body.previousRotationRadians != body.rotationRadians);
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

bool RigidBody2D::ApplyTorque(float torque) {
    if (!std::isfinite(torque)) return false;
    const float accumulated = accumulatedTorque + torque;
    if (!std::isfinite(accumulated)) return false;
    accumulatedTorque = accumulated;
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
    std::map<ECS::EntityID, float> previousRotations;
    world->ForEach<Transform2D, RigidBody2D>(
        [this, world, deltaTime, &previousPositions, &previousRotations](ECS::Entity entity,
                                              Transform2D& transform,
                                              RigidBody2D& body) {
            previousPositions[entity.id] = transform.position;
            const Rotation2D* initialRotation =
                world->GetComponent<Rotation2D>(entity);
            previousRotations[entity.id] = initialRotation
                ? initialRotation->radians : 0.0f;
            if (!std::isfinite(body.inverseMass) || body.inverseMass < 0.0f ||
                !IsFinite(transform.position) || !IsFinite(body.velocity) ||
                !IsFinite(body.accumulatedForce) ||
                !std::isfinite(body.angularVelocity) ||
                !std::isfinite(body.accumulatedTorque) ||
                !std::isfinite(body.inverseInertia) ||
                body.inverseInertia < 0.0f) {
                return;
            }

            if (body.inverseMass > 0.0f) {
                const Math::Vec2 acceleration =
                    m_Gravity + body.accumulatedForce * body.inverseMass;
                const Math::Vec2 velocity =
                    body.velocity + acceleration * deltaTime;
                const Math::Vec2 position =
                    transform.position + velocity * deltaTime;
                if (IsFinite(acceleration) && IsFinite(velocity) &&
                    IsFinite(position)) {
                    body.velocity = velocity;
                    transform.position = position;
                    body.accumulatedForce = {};
                }
            } else {
                body.accumulatedForce = {};
            }

            if (body.inverseInertia > 0.0f) {
                const float angularAcceleration =
                    body.accumulatedTorque * body.inverseInertia;
                const float angularVelocity = body.angularVelocity +
                    angularAcceleration * deltaTime;
                Rotation2D* rotation = world->GetComponent<Rotation2D>(entity);
                const float radians = rotation
                    ? rotation->radians + angularVelocity * deltaTime
                    : 0.0f;
                if (std::isfinite(angularAcceleration) &&
                    std::isfinite(angularVelocity) &&
                    (!rotation || std::isfinite(radians))) {
                    body.angularVelocity = angularVelocity;
                    if (rotation) rotation->radians = radians;
                    body.accumulatedTorque = 0.0f;
                }
            } else {
                body.accumulatedTorque = 0.0f;
            }
        });

    const ContactSolveStats stats =
        ResolveContacts(*world, previousPositions, previousRotations,
                        deltaTime);
    m_LastBroadPhasePairCount = stats.uniqueCandidatePairs;
    m_LastContactIterationCount = stats.iterations;
    m_LastContactSolveConverged = stats.converged;
}

void Physics2DSystem::DescribeAccess(ECS::AccessSpec& access) const {
    access.Write<Transform2D>();
    access.Write<RigidBody2D>();
    access.Read<BoxCollider2D>();
    access.Write<Rotation2D>();
    access.Read<Restitution2D>();
}

} // namespace StonePotatoes::Physics
