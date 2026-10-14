# Physics vertical slice: deterministic 2D fixed-tick integration

Status: initial 2D body integration is implemented; collisions and contacts are
out of scope for this slice.

## Scope

`Physics2DSystem` is an ECS system that integrates `Transform2D` and
`RigidBody2D` using semi-implicit Euler. Each tick applies gravity and the
body's accumulated force to velocity, advances position using the new velocity,
then clears the accumulated force. A body with `inverseMass == 0` is immovable.

`Game::SetFixedStep` owns the simulation clock and catch-up limit. Register the
physics system in the ECS world and let `Game::Update` call the world once per
fixed tick. The physics system rejects invalid/non-positive deltas and skips a
body if its state or calculated next state is non-finite.

## Determinism boundary

For a given executable, initial world state, force application sequence, and
fixed-tick count, the system has no wall-clock reads, random inputs, cross-body
reductions, or unordered iteration. Tests compare the same eight ticks supplied
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
- Non-finite forces and invalid physics deltas do not contaminate body state.

Collision detection, contact resolution, joints, and cross-platform replay
serialization remain future work.
