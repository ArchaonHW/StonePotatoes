# Game runtime and scenes

`StonePotatoes::Game::Game` coordinates one active `Scene` with an externally
owned `ECS::World`. The platform layer remains responsible for opening the
window and polling its events, so the Game library can be used by headless
consumers.

## Lifecycle

1. Initialize the ECS world and set one scene with `Game::SetScene`.
2. Call `Game::Start(world)`; the scene receives `OnEnter`.
3. For each frame, call `Game::Update(deltaSeconds)`. By default, the scene's
   `Update` callback runs once with the frame delta, followed by one ECS world
   update. To use fixed simulation ticks, call `SetFixedStep(stepSeconds,
   maxCatchUpSteps)` before `Start()`; each tick then updates the scene and ECS
   with the same fixed delta.
4. Call `Game::ChangeScene(newScene)` to queue a transition. In variable-delta
   mode, the transition is applied at the next frame update. In fixed-step mode,
   it is applied before the next simulation tick. If multiple transitions are
   queued first, the latest request replaces the earlier one.
5. Call `Game::Stop`; the active scene receives `OnExit`.

`Game` owns scenes but does not own the ECS world. The world must outlive the
running `Game` and should be shut down after `Game::Stop`.

## Window defaults

`GameConfig` supplies default window dimensions and title. The game executable
loads overrides from `engine_config.json`, then maps those settings to the
platform `WindowConfig`. The runtime loop continues until the window requests
close; elapsed-time limits belong in tests or tools, not the normal app loop.

Fixed-step mode bounds catch-up work to `maxCatchUpSteps` per frame. If the
accumulator still contains whole ticks after that budget, those overdue ticks
are dropped while the fractional remainder is retained. This prevents an
overloaded frame from causing unbounded update work. `DisableFixedStep()`
restores variable-delta mode when the game is stopped. The installed-package
consumer exercises both variable- and fixed-step updates through the public
CMake targets, including that scene and ECS updates share the same fixed tick.
It also verifies that a queued variable-delta scene transition is deferred
until `Update`, then calls the old scene's `OnExit` before the new scene's
`OnEnter` and `Update`. Scene resource ownership and richer transition failure
handling remain future work.
