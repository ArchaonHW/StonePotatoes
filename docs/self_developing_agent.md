# Self-Developing AI Agent Architecture
# 自我開發 AI Agent 架構

## Overview | 概述

The Self-Developing AI Agent is a meta-programming system capable of:
自我開發 AI Agent 是一個元程式設計系統，能夠：

- Writing and modifying its own code
  編寫和修改自己的程式碼
- Testing its own implementations
  測試自己的實作
- Optimizing its own performance
  優化自己的效能
- Learning from failures and successes
  從失敗和成功中學習
- Generating new algorithms for IQGPU
  為 IQGPU 生成新演算法

## Core Architecture | 核心架構

```
Self-Developing Agent
自我開發 Agent
│
├── Meta-Programming Layer | 元程式設計層
│   ├── Code Generator | 程式碼生成器
│   ├── Code Parser | 程式碼解析器
│   ├── Code Mutator | 程式碼變異器
│   └── Template System | 模板系統
│
├── Self-Testing Layer | 自我測試層
│   ├── Test Generator | 測試生成器
│   ├── Test Executor | 測試執行器
│   ├── Coverage Analyzer | 覆蓋率分析器
│   └── Bug Detector | 錯誤檢測器
│
├── Self-Optimization Layer | 自我優化層
│   ├── Performance Profiler | 效能分析器
│   ├── Bottleneck Detector | 瓶頸檢測器
│   ├── Optimization Engine | 優化引擎
│   └── Benchmark System | 基準測試系統
│
├── Learning Layer | 學習層
│   ├── Experience Memory | 經驗記憶
│   ├── Pattern Recognition | 模式識別
│   ├── Success/Failure Analysis | 成功/失敗分析
│   └── Knowledge Graph | 知識圖譜
│
├── IQGPU Integration Layer | IQGPU 整合層
│   ├── GPU Code Generator | GPU 程式碼生成器
│   ├── Kernel Optimizer | 核心優化器
│   ├── Memory Manager | 記憶體管理器
│   └── Compute Scheduler | 計算排程器
│
└── Sandbox Layer | 沙盒層
    ├── Isolated Environment | 隔離環境
    ├── Resource Limits | 資源限制
    ├── Safety Monitors | 安全監控器
    └── Rollback System | 回滾系統
```

## Agent Capabilities | Agent 能力

### 1. Code Generation | 程式碼生成

The agent can generate:
Agent 可以生成：

- C++ code for core systems
  核心系統的 C++ 程式碼
- GPU compute shaders
  GPU 計算著色器
- AI algorithm implementations
  AI 演算法實作
- Optimization kernels
  優化核心
- Test cases
  測試案例

### 2. Self-Testing | 自我測試

The agent can:
Agent 可以：

- Generate unit tests
  生成單元測試
- Run integration tests
  執行整合測試
- Detect bugs
  檢測錯誤
- Validate correctness
  驗證正確性
- Measure test coverage
  測量測試覆蓋率

### 3. Self-Optimization | 自我優化

The agent can:
Agent 可以：

- Profile performance
  分析效能
- Identify bottlenecks
  識別瓶頸
- Optimize algorithms
  優化演算法
- Parallelize code
  平行化程式碼
- Reduce memory usage
  減少記憶體使用

### 4. Learning | 學習

The agent can:
Agent 可以：

- Remember successful patterns
  記住成功的模式
- Learn from failures
  從失敗中學習
- Build knowledge base
  建立知識庫
- Improve over time
- 隨時間改進
- Adapt to new requirements
  適應新需求

## Development Phases | 開發階段

### Phase 1: Foundation | 第一階段：基礎
- Meta-programming framework
  元程式設計框架
- Code generation templates
  程式碼生成模板
- Basic sandbox environment
  基礎沙盒環境

### Phase 2: Self-Testing | 第二階段：自我測試
- Test generation system
  測試生成系統
- Automated testing pipeline
  自動化測試管線
- Bug detection
  錯誤檢測

### Phase 3: Self-Optimization | 第三階段：自我優化
- Performance profiling
  效能分析
- Optimization strategies
  優化策略
- GPU kernel optimization
  GPU 核心優化

### Phase 4: Learning | 第四階段：學習
- Experience memory
  經驗記憶
- Pattern recognition
  模式識別
- Knowledge graph
  知識圖譜

### Phase 5: IQGPU Integration | 第五階段：IQGPU 整合
- GPU code generation
  GPU 程式碼生成
- Quantum-inspired algorithms
  量子啟發演算法
- Distributed optimization
  分散式優化

## Safety Considerations | 安全考量

### Resource Limits | 資源限制
- CPU time limits
  CPU 時間限制
- Memory limits
  記憶體限制
- Disk space limits
  磁碟空間限制
- Network limits
  網路限制

### Code Review | 程式碼審查
- Human review required for critical changes
- 關鍵變更需要人工審查
- Automated code analysis
  自動化程式碼分析
- Security scanning
  安全掃描

### Rollback Mechanisms | 回滾機制
- Version control integration
  版本控制整合
- Automatic rollback on failure
  失敗時自動回滾
- Manual intervention capability
  人工干預能力

## Implementation Plan | 實作計畫

See [ROADMAP.md](../ROADMAP.md) for the overall project roadmap.
整體專案路線圖請參閱 [ROADMAP.md](../ROADMAP.md)。

This system will be integrated into Phase 8 (GPU Massive Simulation) and beyond.
此系統將整合到第八階段（GPU 大規模模擬）及之後。

---

## Current Implementation Status | 目前實作狀態

Build the standalone tool with `cmake --build build --config Release --target StonePotatoesSelfDevelopingAgent`.
Run it from a writable working directory with a goal, for example:
`build/tools/self_developing_agent/Release/StonePotatoesSelfDevelopingAgent.exe "Analyze ECS performance"`.
It writes generated artifacts to `agent_workspace/` under the current working directory.

> Per the "No Fake Features" principle, this section states precisely what is
> implemented today versus what remains planned.
> 依據「No Fake Features」原則，本章節精確說明目前已實作與仍屬計畫的功能。

### Implemented | 已實作

| Subsystem 子系統 | Status 狀態 | Details 細節 |
|---|---|---|
| `MetaProgrammingAgent` | ✅ Working | Orchestrates a real 7-phase cycle: Analyze → Generate → Write → Static-check → Compile → Optimize → Learn. 協調真實七階段週期：分析 → 生成 → 寫入 → 靜態檢查 → 編譯 → 優化 → 學習。 |
| `CodeGenerator` | ✅ Working | Parses a goal string into a `GenerationRequest` and emits a compilable C++ file (function + `main()` harness), a test harness `.cpp`, and an HLSL compute-shader skeleton. 將目標字串解析為 `GenerationRequest`，輸出可編譯的 C++ 檔案（函式 + `main()` 測試程式）、測試 `.cpp` 與 HLSL 計算著色器骨架。 |
| `CodeParser` | ✅ Working | Real static analysis over source text: counts code/comment/blank lines, functions, classes, loops, branches, allocations, lambdas, TODO/FIXME markers, and collects `#include`s. 對原始碼做真實靜態分析：統計程式碼/註解/空行、函式、類別、迴圈、分支、配置、lambda、TODO/FIXME 標記與 `#include`。 |
| `CodeMutator` | ✅ Working | Real text transforms: strip comments, inject timing instrumentation, `malloc`→`std::vector`, `const` annotation, `reserve()` hints. 真實文字轉換：移除註解、注入計時、`malloc`→`std::vector`、`const` 標註、`reserve()` 提示。 |
| `SelfTester` | ✅ Working | Real compiler detection (MSVC via `vswhere` + `vcvars64.bat`, `g++`, `clang++`), real compile + run with timeouts, real static bug checks (unbalanced braces, `new`/`malloc` without `delete`/`free`, `while(true)` without `break`, self-assignment). 真實編譯器偵測（MSVC 經 `vswhere` + `vcvars64.bat`、`g++`、`clang++`），真實編譯與執行（含逾時），真實靜態錯誤檢查（大括號不平衡、`new`/`malloc` 無對應釋放、`while(true)` 無 `break`、自我賦值）。 |
| `SelfOptimizer` | ✅ Working | Real heuristic analysis: heap allocation in loops (Critical), `std::endl` in loops, `+=` in loops when `std::string` is present, `push_back` without `reserve()`, vector copies; applies safe transforms automatically. 真實啟發式分析：迴圈內堆積配置（Critical）、迴圈內 `std::endl`、存在 `std::string` 時迴圈內 `+=`、`push_back` 無 `reserve()`、vector 複製；自動套用安全轉換。 |
| `LearningSystem` | ✅ Working | Persists experience records to `agent_experience.jsonl` (JSONL), loads them back, computes success rate, and suggests prior approaches by goal keyword. 將經驗記錄持久化到 `agent_experience.jsonl`（JSONL），可重新載入、計算成功率並依目標關鍵字建議先前方法。 |
| `SandboxEnvironment` | ✅ Working | Executes a generated binary in a dedicated `agent_workspace/` directory, captures stdout/stderr, enforces a wall-clock timeout, supports working-directory restriction. 在專用 `agent_workspace/` 目錄執行生成的二進位檔，擷取 stdout/stderr，強制時鐘逾時，支援工作目錄限制。 |

### Verified Behavior | 已驗證行為

On the development machine (MSVC, Windows), one `DevelopmentCycle()` measurably:
在開發機器上（MSVC, Windows），一次 `DevelopmentCycle()` 實際測得：

1. Generated a 17-line C++ program from the goal string.
   由目標字串生成 17 行 C++ 程式。
2. Generated a `_test.cpp` harness file.
   生成 `_test.cpp` 測試檔案。
3. Compiled it with MSVC in ~3.7 s.
   以 MSVC 約 3.7 秒編譯。
4. Executed the binary successfully (exit 0) in ~1.4 s.
   約 1.4 秒內成功執行（exit 0）。
5. Persisted experience records; `selfImprovementScore` reflects the measured
   success rate (e.g., 0.50 after 1 success / 2 records).
   持久化經驗記錄；`selfImprovementScore` 反映實測成功率（例如 1 成功 / 2 記錄後為 0.50）。

Artifacts are written under `agent_workspace/` next to the executable.
產出物寫入執行檔旁的 `agent_workspace/`。

### Not Yet Implemented | 尚未實作

| Feature 功能 | Status 狀態 |
|---|---|
| LLM-driven code synthesis（以語言模型驅動的程式合成） | 📋 Planned — current generation is template/heuristic based |
| Compiling *and running* the generated test harness | 📋 Planned — test files are generated but not yet compiled/executed automatically |
| Applying optimized code back to the engine source | 📋 Planned — optimizations are analyzed/suggested, not auto-merged |
| Hard OS-level sandboxing (job objects, rlimits, seccomp) | 📋 Planned — current sandbox is workspace + timeout only |
| Rollback / git integration | 📋 Planned |
| Knowledge graph, deep pattern recognition | 📋 Planned — current learning is flat JSONL records |
| IQGPU / GPU-kernel compile-test loop | 📋 Planned — HLSL generation exists; no GPU toolchain invocation yet |
| Coverage measurement | 📋 Planned |
| Multi-agent / distributed self-development | 📋 Planned |

### Known Limitations | 已知限制

- Compilation spawns `cl`/`g++` via the shell; first MSVC invocation per cycle
  reloads `vcvars64.bat` (~3–4 s overhead per compile).
  編譯透過 shell 呼叫 `cl`/`g++`；每次週期的首次 MSVC 呼叫會重新載入
  `vcvars64.bat`（每次編譯約 3–4 秒額外負擔）。
- The sandbox enforces timeouts and working-directory isolation only; it does
  **not** restrict memory, file system, or network access of the child process.
  沙盒僅強制逾時與工作目錄隔離；**不**限制子行程的記憶體、檔案系統或網路存取。
- Goal parsing is keyword-based; arbitrary natural-language goals are reduced to
  a generic generated program.
  目標解析基於關鍵字；任意自然語言目標會被退化為通用生成程式。
- `selfImprovementScore` is a simple success-rate metric, not a learned model.
  `selfImprovementScore` 是簡單的成功率指標，而非學習模型。
