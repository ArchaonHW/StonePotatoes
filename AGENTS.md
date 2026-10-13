# AGENTS.md

## PotatoEngine Ports (MIT License)

The following components are ported from PotatoEngine (D:\HWC\PotatoEngine), also MIT-licensed by ArchaonHW:

- **Core/Profiler** → `src/core/profiling/Profiler.{h,cpp}` — Zone timing + Chrome trace export
- **MathUtils/Vec2, Vec3** → `src/math/Vec2.h, Vec3.h` — Minimal vector types
- **MathUtils/Pathfinding** → `src/math/Pathfinding.h` — Grid A* + FlowField (lockstep-safe)
- **MathUtils/QuasiRandom** → `src/math/QuasiRandom.h` — Low-discrepancy sequences (R2, Halton, Poisson-disk)
- **MathUtils/CurlNoise** → `src/math/CurlNoise.h` — Divergence-free turbulence field
- **Simulation/SystemDynamics** → `src/civilization/economic/SystemDynamics.{h,cpp}` — Stock-flow simulation (RTS economy)
- **Networking/LockstepSync** → `src/networking/core/LockstepSync.{h,cpp}` — Deterministic lockstep synchronization

All ported files include SPDX identifiers and provenance headers. See `THIRD_PARTY_NOTICES.md` for full details.

## web3-agent (Java, standalone Maven module)

- Toolchain: `D:\HWC\.tools` (JDK 25 Temurin, Maven 3.9.11). In Git Bash:
  `export JAVA_HOME=/d/HWC/.tools/jdk/jdk-25.0.4.1+1; export PATH=$JAVA_HOME/bin:/d/HWC/.tools/apache-maven-3.9.11/bin:$PATH`
- Test: `cd web3-agent && mvn -B test` (15 offline tests, no chain or LLM needed)
- Integration tests (optional): `ANVIL_TEST=1 mvn -B test` (requires Anvil on 127.0.0.1:8545), `LLM_TEST=1 mvn -B test` (requires llama.cpp on 127.0.0.1:11434)
- Package: `mvn -B package` -> `web3-agent/target/web3-agent-0.1.0.jar`
- JSON uses Jackson 3 (`tools.jackson.*`, pulled in by web3j 6), not `com.fasterxml.jackson`.
- Not part of the CMake build; independent of the C++ engine.
- Features: EIP-1559 transactions, receipt tracking, contract calls/deployment (ERC-20), ABI signatures, P2P gossip, content store, OpenAI-compatible LLM provider.
- Real environment: Foundry 1.8.5 at `D:\HWC\.tools\foundry`, llama.cpp v0.6.0 at `D:\HWC\.tools\llama`, Qwen 2.5 3B Q4_K_M at `D:\HWC\.tools\_dl\qwen2.5-3b-instruct-q4_k_m.gguf`.
