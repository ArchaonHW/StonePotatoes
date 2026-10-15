# Physics vertical slice: deterministic 2D fixed-tick bodies and box contacts

Status: fixed-tick 2D body integration and discrete axis-aligned box contacts
are implemented. This is a deliberately small collision slice, not a general
purpose rigid-body solver.

## Scope

`Physics2DSystem` is an ECS system that integrates `Transform2D` and
`RigidBody2D` using semi-implicit Euler. Each tick applies gravity and the
body's accumulated force to velocity, advances position using the new velocity,
then clears the accumulated force. A body with `inverseMass == 0` is immovable.
Entities that also have a `BoxCollider2D` participate in collision handling;
the collider is an axis-aligned box centered on the transform with positive
half-extents.

After integration, the system sorts valid collider bodies by `Entity::id` and
places each AABB into a deterministic 2D grid with 2-unit cells. Each collider
occupies every cell touched by its bounds. Candidate pairs are deduplicated and
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
not converged. Overlapping boxes are separated along the axis with the smaller
penetration (ties choose X), with displacement weighted by inverse mass. A
zero-restitution normal impulse stops closing normal velocity; tangential
velocity is unchanged. Invalid or non-finite body and collider state is skipped
by collision handling.

`Game::SetFixedStep` owns the simulation clock and catch-up limit. Register the
physics system in the ECS world and let `Game::Update` call the world once per
fixed tick. The physics system rejects invalid/non-positive deltas and skips a
body if its state or calculated next state is non-finite.

## Determinism boundary

For a given executable, initial world state, force application sequence, and
fixed-tick count, the system has no wall-clock reads, random inputs, or
unordered collision-pair iteration. Collision resolution is a single ordered
pass; with three or more simultaneously contacting bodies, changing entity IDs
can change the result. Tests compare the same tick sequence supplied through
different render-frame partitions. This is repeatability within the same
platform/build; cross-platform bitwise lockstep and floating-point
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
- The same collision setup produces equal results across different render-frame
  partitions.
- A 256-body separated scene sends substantially fewer pairs to narrow phase
  than an all-pairs scan, while oversized colliders still find their contacts.
- A fixed-seed scene spanning negative coordinates and exact cell boundaries
  reports exactly the isolated overlapping pairs and resolves each contact.
- A stacked contact created by positional correction is found on a later
  iteration and settles within the iteration limit.
- Non-finite forces and invalid physics deltas do not contaminate body state.

Rotated shapes, friction, joints, continuous collision detection (including
tunneling at high speed), iterative multi-contact solving, and cross-platform
replay serialization remain future work.
