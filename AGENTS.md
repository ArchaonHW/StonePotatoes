# AGENTS.md

> Near-term roadmap & component boundaries (engine vs self_developing_agent vs web3-agent vs web3-ide): [docs/ROADMAP-near-term.md](docs/ROADMAP-near-term.md)

## PotatoEngine Ports (MIT License) — planned, not yet in this repo

> **Status:** none of the files below exist in the repository yet. This is a porting
> plan, not an inventory. When a component is actually ported, add the file with an
> SPDX identifier + provenance header, create `THIRD_PARTY_NOTICES.md`, and move the
> entry to a "Ported" list.
> **狀態：** 下列檔案目前皆尚未存在於本 repo，這是移植計畫而非現況清單。

Planned targets (source: PotatoEngine, also MIT-licensed by ArchaonHW):

- Core/Profiler → `src/core/profiling/Profiler.{h,cpp}` — zone timing + Chrome trace export
- MathUtils/Vec2, Vec3 → `src/math/Vec2.h`, `Vec3.h`
- MathUtils/Pathfinding → `src/math/Pathfinding.h` — grid A* + FlowField (lockstep-safe)
- MathUtils/QuasiRandom → `src/math/QuasiRandom.h` — R2, Halton, Poisson-disk
- MathUtils/CurlNoise → `src/math/CurlNoise.h`
- Simulation/SystemDynamics → `src/civilization/economic/SystemDynamics.{h,cpp}`
- Networking/LockstepSync → `src/networking/core/LockstepSync.{h,cpp}`

## C++ engine build

- Linux deps: `cmake g++ build-essential xorg-dev libwayland-dev libxkbcommon-dev` (GLFW is fetched by CMake and needs X11/Wayland headers).
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSTONEPOTATOES_BUILD_EDITOR=OFF && cmake --build build -j && ctest --test-dir build --output-on-failure`
- Vulkan is optional: without the SDK only the CPU backend is built and `vulkan_unit` skips. Force CPU-only with `-DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON` (as CI does).
- Coverage (optional, GCC/Clang): `-DSTONEPOTATOES_ENABLE_COVERAGE=ON`, or run `./tools/ci/coverage.sh` (needs `lcov`) → `coverage/lcov.info`, `coverage/html/`. CI uploads it as the `cpp-coverage-lcov` artifact.
- `vulkan_unit` exits 77 without a Vulkan device; ctest reports it as Skipped (`SKIP_RETURN_CODE 77`).
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
