# Physics vertical slice: deterministic 2D fixed-tick bodies and box contacts

Status: fixed-tick 2D body integration, discrete oriented-box contacts,
Coulomb friction, configurable restitution, and event-driven swept-AABB
contacts between static and dynamic axis-aligned boxes are implemented.
This is a deliberately small collision slice, not a general-purpose rigid-body
solver.

## Scope

`Physics2DSystem` is an ECS system that integrates `Transform2D` and
`RigidBody2D` using semi-implicit Euler. Each tick applies gravity and the
body's accumulated force to velocity, advances position using the new velocity,
then clears the accumulated force. A body with `inverseMass == 0` is immovable.
Entities that also have a `BoxCollider2D` participate in collision handling;
the collider is centered on the transform with positive half-extents. An
optional `Rotation2D` component supplies a finite, fixed orientation in
radians. Without it, the box is axis-aligned. Rotation has no angular velocity
or integration.

After integration, the system sorts valid collider bodies by `Entity::id` and
places each box's world-space AABB into a deterministic 2D grid with 2-unit
cells. Each collider occupies every cell touched by its bounds. Candidate pairs are deduplicated and
sorted by entity ID before narrow-phase checks. Colliders spanning more than
256 cells (or outside the grid's integer coordinate range) conservatively pair
with every body, avoiding unbounded grid allocation. The public
`GetLastBroadPhasePairCount()` diagnostic reports unique pairs sent to the
narrow phase during the last tick.

For up to 16 solver iterations, the grid and sorted candidate list are rebuilt
from the latest positions. This lets positional correction expose additional
contacts during the same tick. Iteration stops when a full pass makes no state
change; penetrations at or below `0.001` world units are tolerated to avoid
chasing floating-point residue. `GetLastContactIterationCount()` and
`DidLastContactSolveConverge()` report the last solve's progress; a solve that
still changes state on the final allowed pass is conservatively reported as
not converged. The narrow phase uses the four box axes SAT and separates
overlapping boxes along the least-penetration axis, with displacement weighted
by inverse mass. A
normal impulse applies the contact restitution. `Restitution2D` is an optional
POD component with a coefficient in `[0, 1]`; absent components behave as zero.
The contact uses the larger coefficient from its two bodies, so a moving body
can opt into bounce without requiring static geometry to carry a material.
With `e` as the combined coefficient and `vN` as the closing normal speed, the
impulse magnitude is `-(1 + e) * vN / inverseMassSum`. Zero preserves the
existing inelastic behavior. When the normal impulse is nonzero, the solver
applies Coulomb friction along the contact tangent: it cancels tangential
velocity up to the static-friction limit, then uses the dynamic-friction limit
for sliding. Each limit is the geometric mean of the two colliders'
coefficients multiplied by the normal impulse. Both friction coefficients
default to zero, preserving frictionless contacts; dynamic friction must be no
greater than static friction. Invalid or non-finite body, collider, or material
state is skipped by collision handling.

Before the discrete solver, an event-driven swept-AABB pass uses each
axis-aligned collider's previous and integrated positions to find the earliest
impact among static-dynamic and dynamic-dynamic pairs during the remaining tick. Bodies
advance to that time of impact in entity-ID pair order, then receive the same
restitution-aware normal and Coulomb friction impulses as discrete contacts;
dynamic motion is distributed by inverse mass. The remaining tick is
simulated again with the updated velocities. A bounded event budget prevents
unbounded work; if it is exhausted, bodies stay at the last resolved impact
and the unprocessed time is discarded to avoid tunneling. Initial overlaps
remain the discrete solver's responsibility. Rotated boxes use the discrete
OBB solver; rotated swept time-of-impact and angular dynamics are not included.

`Game::SetFixedStep` owns the simulation clock and catch-up limit. Register the
physics system in the ECS world and let `Game::Update` call the world once per
fixed tick. The physics system rejects invalid/non-positive deltas and skips a
body if its state or calculated next state is non-finite.

## Determinism boundary

For a given executable, initial world state, force application sequence, and
fixed-tick count, the system has no wall-clock reads, random inputs, or
unordered collision-pair iteration. Each bounded solve iteration uses sorted
entity-ID pairs; with three or more simultaneously contacting bodies, changing
entity IDs can change the result. Tests compare the same tick sequence supplied
through different render-frame partitions. This is repeatability within the
same platform/build; cross-platform bitwise lockstep and floating-point
canonicalization are not promised.

## Acceptance

- Fixed-step game updates leave physics state unchanged until a full tick is
  accumulated.
- Dynamic bodies integrate gravity and one-tick force with semi-implicit Euler.
- Static bodies do not move and queued force is discarded.
- Equal initial states and equal fixed-tick counts produce equal results when
  render-frame deltas are partitioned differently.
- A dynamic box is separated from a static box and its closing normal velocity
  is removed; two equal-mass boxes stop in a head-on contact.
- A body with restitution 0.5 rebounds from a static wall at half its incoming
  normal speed for both discrete overlap contacts and swept-AABB impacts.
- Invalid restitution outside `[0, 1]` excludes that body from collision
  handling without changing its integrated motion.
- Coulomb friction slows a sliding box according to the dynamic coefficient
  and cancels a low tangential speed within the static-friction limit.
- Fast boxes moving in either direction stop at static walls without crossing
  them, while tangential motion continues for the remainder of the tick.
- Fast equal-mass dynamic boxes exchange normal velocity at first contact
  without crossing; unequal inverse masses produce the expected shared normal
  velocity after impact.
- A separately configured installed-package consumer can use the public
  `StonePotatoes::Physics` target to stop a fast box at a static wall and apply
  friction to a sliding box, resolve a fast dynamic-dynamic head-on impact,
  apply restitution to a bouncing box, and deterministically separate two
  coincident dynamic boxes within the bounded contact solve, without relying
  on the repository's unit target.
- Rotated thin boxes whose world AABBs overlap but whose OBBs are separated do
  not collide; overlapping OBBs separate along their SAT minimum-penetration
  axis.
- The same collision setup produces equal results across different render-frame
  partitions.
- A 256-body separated scene sends substantially fewer pairs to narrow phase
  than an all-pairs scan, while oversized colliders still find their contacts.
- A fixed-seed scene spanning negative coordinates and exact cell boundaries
  reports exactly the isolated overlapping pairs and resolves each contact.
- A stacked contact created by positional correction is found on a later
  iteration and settles within the iteration limit.
- Non-finite forces and invalid physics deltas do not contaminate body state.

The discrete solver separates coincident initial boxes using entity-ID order.
Rotated swept time-of-impact, angular dynamics, more advanced initial-overlap
recovery beyond the bounded solver, joints, and cross-platform replay
serialization remain future work.
