# AGENTS.md

> Near-term roadmap & component boundaries (engine vs self_developing_agent vs web3-agent vs web3-ide): [docs/ROADMAP-near-term.md](docs/ROADMAP-near-term.md)

## PotatoEngine Ports (MIT License) — ported

> **Status:** these components are present in this repository. Keep provenance and license notices in sync with `THIRD_PARTY_NOTICES.md` when changing them.
> **狀態：** 以下移植元件已存在於本 repo；修改時須同步維護來源與 `THIRD_PARTY_NOTICES.md`。

Ported from PotatoEngine by ArchaonHW (MIT):

- Core/Profiler → `src/core/profiling/Profiler.{h,cpp}`
- MathUtils/Vec2, Vec3 → `src/math/Vec2.h`, `Vec3.h`
- MathUtils/Pathfinding → `src/math/Pathfinding.h`
- MathUtils/QuasiRandom → `src/math/QuasiRandom.h`
- MathUtils/CurlNoise → `src/math/CurlNoise.h`
- Simulation/SystemDynamics → `src/civilization/economic/SystemDynamics.{h,cpp}`
- Networking/LockstepSync → `src/networking/core/LockstepSync.{h,cpp}`

## C++ engine build

- Linux deps: `cmake g++ build-essential xorg-dev libwayland-dev libxkbcommon-dev` (GLFW is fetched by CMake and needs X11/Wayland headers).
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSTONEPOTATOES_BUILD_EDITOR=OFF && cmake --build build -j && ctest --test-dir build --output-on-failure`
- Vulkan is optional: without the SDK only the CPU backend is built and `vulkan_unit` skips. Force CPU-only with `-DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON` (as CI does).
- Coverage (optional, GCC/Clang): `-DSTONEPOTATOES_ENABLE_COVERAGE=ON`, or run `./tools/ci/coverage.sh` (needs `lcov`) → `coverage/lcov.info`, `coverage/html/`. CI uploads it as the `cpp-coverage-lcov` artifact.
- `vulkan_unit` exits 77 without a Vulkan device; ctest reports it as Skipped (`SKIP_RETURN_CODE 77`).
- With the Vulkan SDK installed, build the windowed clear/present demo with `cmake --build build --config Release --target StonePotatoesVulkanPresentationSmoke`; CI compiles this target in the software-device job. The demo needs a display to run.
- With the Vulkan SDK installed, build `StonePotatoesVulkanComputeRendererDemo` to see the production renderer and SAXPY compute backend share one `VulkanContext`/`VkDevice`; it needs a display to run.
- The Linux Vulkan CI uses Xvfb to run `StonePotatoesVulkanComputeRendererDemo --frames 3` for shared-device compute/presentation and clean shutdown, plus `StonePotatoesVulkanPresentationSmoke --frames 3 --resize-at-frame 1` to verify swapchain recreation and a later presented frame.
- `install_consumer` validates the installed headless package by building a separate CMake consumer against `StonePotatoes::Physics`, `StonePotatoes::Civilization`, `StonePotatoes::Networking`, `StonePotatoes::ECS`, `StonePotatoes::Game`, and `StonePotatoes::Data`, exercising fixed-tick physics force/torque integration, angular impulses from off-center impact, rotation-swept obstacle contacts, swept wall, dynamic-dynamic, and rotated-box contacts, deterministic recovery of coincident initial boxes, friction, stock-flow economics and logistic population dynamics, lockstep tick gating/peer commands/checksum mismatch reporting, scene lifecycle, world update, save/load, and asset caching.
- Game runtime responsibilities and the scene transition lifecycle are documented in `docs/game-runtime.md`.
- The game executable is built to `build/bin/<Config>/StonePotatoes[.exe]` with multi-config generators or `build/bin/StonePotatoes` with single-config generators; `cmake --install` stages the runtime and headless SDK, and `cpack` writes a platform ZIP/TGZ package under `build/packages/`. The self-developing agent remains a developer tool and is not included in the game package.
- CI: `.github/workflows/ci.yml`.

## web3-agent (Java, standalone Maven module)

- Toolchain: JDK 25 + Maven 3.9.x on `PATH` (any OS). Set `JAVA_HOME` to your JDK 25.
  - Example (owner's Windows/Git Bash layout, adjust to yours):
    `export JAVA_HOME=/d/HWC/.tools/jdk/jdk-25.0.4.1+1; export PATH=$JAVA_HOME/bin:/d/HWC/.tools/apache-maven-3.9.11/bin:$PATH`
  - Linux example: `export JAVA_HOME=/usr/lib/jvm/java-25-openjdk-amd64`
- Test: `cd web3-agent && mvn -B test` (offline; chain/LLM tests are skipped unless enabled below)
- Integration tests (optional): `ANVIL_TEST=1 mvn -B test` (requires Anvil on 127.0.0.1:8545), `LLM_TEST=1 mvn -B test` (requires llama.cpp on 127.0.0.1:11434)
- Package: `mvn -B package` -> `web3-agent/target/web3-agent-0.1.0.jar`
- JSON uses Jackson 3 (`tools.jackson.*`, pulled in by web3j 6), not `com.fasterxml.jackson`.
- Not part of the CMake build; independent of the C++ engine.
- Features: EIP-1559 transactions, receipt tracking, contract calls/deployment (ERC-20), ABI signatures, P2P gossip, content store, OpenAI-compatible LLM provider.
- Reference environment (example paths from the owner's Windows machine; install anywhere): Foundry 1.8.5 (`anvil`), llama.cpp v0.6.0, Qwen 2.5 3B Q4_K_M GGUF — e.g. `D:\HWC\.tools\foundry`, `D:\HWC\.tools\llama`, `D:\HWC\.tools\_dl\qwen2.5-3b-instruct-q4_k_m.gguf`.
