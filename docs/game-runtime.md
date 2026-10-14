# Game runtime and scenes

`StonePotatoes::Game::Game` coordinates one active `Scene` with an externally
owned `ECS::World`. The platform layer remains responsible for opening the
window and polling its events, so the Game library can be used by headless
consumers.

## Lifecycle

1. Initialize the ECS world and set one scene with `Game::SetScene`.
2. Call `Game::Start(world)`; the scene receives `OnEnter`.
3. For each frame, call `Game::Update(deltaSeconds)`. The scene's `Update`
   callback runs, followed by the ECS world's system update.
4. Call `Game::ChangeScene(newScene)` to queue a transition. The old scene gets
   `OnExit` and the new scene gets `OnEnter` at the start of the next update.
   If multiple transitions are queued before that update, the latest request
   replaces the earlier one.
5. Call `Game::Stop`; the active scene receives `OnExit`.

`Game` owns scenes but does not own the ECS world. The world must outlive the
running `Game` and should be shut down after `Game::Stop`.

## Window defaults

`GameConfig` supplies default window dimensions and title. The game executable
loads overrides from `engine_config.json`, then maps those settings to the
platform `WindowConfig`. The runtime loop continues until the window requests
close; elapsed-time limits belong in tests or tools, not the normal app loop.

The current runtime passes variable frame delta to scene and ECS updates. A
fixed-step accumulator, scene resource ownership, and richer transition failure
handling remain future work.
