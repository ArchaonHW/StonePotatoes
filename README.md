# StonePotatoes

> Build Worlds That Think.
> 建造會思考的世界。

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.20+-blue.svg)](https://cmake.org/)
[![Status: Early Development](https://img.shields.io/badge/Status-Early%20Development-orange.svg)](#current-status)
[![Architecture: Active](https://img.shields.io/badge/Architecture-Active-green.svg)](#architecture)
[![IQGPU: Experimental](https://img.shields.io/badge/IQGPU-Experimental-purple.svg)](#intelligent-quantum-gpu-development-system)

---

StonePotatoes is an AI-native, GPU-first, quantum-inspired game and world simulation engine designed for massive-scale intelligent agents, procedural worlds, emergent civilizations, and next-generation interactive experiences.

StonePotatoes 是一個 AI 原生、GPU 優先、量子啟發的遊戲與世界模擬引擎，專為大規模智能代理、程序化世界、湧現文明與下一代互動體驗而設計。

It is not just a game engine. It is an intelligent world computation platform that unifies:

它不僅僅是一個遊戲引擎。它是一個智能世界計算平台，統一了：

- **Game Engine** - Interactive experiences and gameplay
  **遊戲引擎** - 互動體驗與遊戲玩法
- **AI Engine** - Autonomous agent runtime and decision systems
  **AI 引擎** - 自主代理運行時與決策系統
- **World Simulation Engine** - Living, evolving worlds with procedural generation
  **世界模擬引擎** - 具有程序化生成的活著、演化的世界
- **Civilization Engine** - Political, economic, military, and cultural systems
  **文明引擎** - 政治、經濟、軍事與文化系統
- **GPU Compute Platform** - High-performance parallel computation
  **GPU 計算平台** - 高效能平行計算
- **Agent Runtime** - Multi-layered AI hierarchy for autonomous behavior
  **代理運行時** - 用於自主行為的多層 AI 階層
- **Intelligent Quantum GPU Development System (IQGPU)** - Unified compute abstraction layer
  **智能量子 GPU 開發系統 (IQGPU)** - 統一計算抽象層

---

## Vision | 願景

**Build Worlds That Think.**
**建造會思考的世界。**

StonePotatoes aims to create worlds where:

StonePotatoes 旨在創造這樣的世界：

- Players enter a world that is already running
  玩家進入一個已經運行的世界
- The world does not exist because of the player
  世界不是因為玩家而存在
- The world exists first
  世界先存在
- The player enters it
  玩家進入其中
- After the player leaves, the world continues to run
  玩家離開後，世界繼續運行

**A Living World. A Living Civilization. A Living History. A Living Game.**
**一個活著的世界。一個活著的文明。一個活著的歷史。一個活著的遊戲。**

---

## Current Status | 目前狀態

**Phase 1 — Core Engine** (In Progress)
**第一階段 — 核心引擎**（進行中）

This project is in early development. See [ROADMAP.md](ROADMAP.md) for the complete development plan.

此專案處於早期開發階段。完整開發計畫請參閱 [ROADMAP.md](ROADMAP.md)。

近期里程碑、issue 對照與各元件（引擎 / self_developing_agent / web3-agent / web3-ide）關係：[docs/ROADMAP-near-term.md](docs/ROADMAP-near-term.md)
Game/Scene lifecycle and configuration: [docs/game-runtime.md](docs/game-runtime.md)

### Implemented ✓ | 已實作 ✓
- [x] Core platform, logging, configuration, memory allocators, and plugin manager
  核心平台、日誌、設定、記憶體配置器與外掛管理器
- [x] ECS with archetypes, queries, events, resources, command buffer, and binary world persistence
  ECS 原型、查詢、事件、資源、指令緩衝區與二進位世界持久化
- [x] Parallel system scheduling and unit-tested CPU compute backend
  平行系統排程與具單元測試的 CPU 計算後端
- [x] NPC behavior slice and procedural world streaming
  NPC 行為垂直切片與程序化世界串流
- [x] Optional Vulkan compute backend and ported PotatoEngine utilities
  可選 Vulkan 計算後端與 PotatoEngine 移植工具
- [x] C++20 CMake build and standalone Java Web3 agent
  C++20 CMake 建置與獨立 Java Web3 agent
- [x] Game executable output at `build/bin/` and CPack ZIP/TGZ distribution
  遊戲執行檔固定輸出至 `build/bin/`，並支援 CPack ZIP/TGZ 發行套件

### In Progress 🚧 | 進行中 🚧
- [ ] v0.1 engine productization: fixed-step simulation and complete data module; world save format v1 is documented and validates malformed files (headless ECS + Game/Scene lifecycle and deferred scene switching are available)
  v0.1 引擎產品化：固定步長模擬與完整資料模組；world save v1 已文件化並驗證損壞檔案（headless ECS、Game/Scene lifecycle 與延後切換場景已可使用）
- [ ] Rendering, deterministic physics, and full civilization/networking systems; partial installed C++ slices include `SDModel` and `LockstepSync`
  渲染、決定性物理與完整文明/網路系統仍在進行；已提供可安裝的 `SDModel` 與 `LockstepSync` 部分切片
- [x] Game/window settings from `engine_config.json` and a user-driven runtime loop
  從 `engine_config.json` 載入遊戲/視窗設定，並由使用者控制執行主迴圈

### Not Started 📋 | 尚未開始 📋
- [ ] Editor and modding workflow
  編輯器與 Mod 工作流程
- [ ] Cloud/distributed simulation and production-scale content pipeline
  雲端/分散式模擬與量產級內容管線
---

## Intelligent Quantum GPU Development System (IQGPU) | 智能量子 GPU 開發系統 (IQGPU)

**StonePotatoes introduces IQGPU — a unified compute abstraction layer designed to integrate GPU compute, AI inference, massive parallel simulation, tensor computation, neural processing, procedural generation, physics, agent simulation, world simulation, and future quantum-inspired algorithms.**

**StonePotatoes 引入 IQGPU — 一個統一計算抽象層，旨在整合 GPU 計算、AI 推論、大規模平行模擬、張量計算、神經處理、程序化生成、物理、代理模擬、世界模擬以及未來的量子啟發演算法。**

### Architecture Concept | 架構概念

```
StonePotatoes Runtime
        │
        ▼
    IQGPU Layer
        │
 ┌──────┼────────┐
 ▼      ▼        ▼
GPU    AI       Simulation
Compute Compute Compute
 │      │        │
 └──────┼────────┘
        ▼
Unified Compute Runtime
```

### Design Goals | 設計目標

IQGPU is designed to unify:

IQGPU 旨在統一：

- **GPU Compute** - Parallel computation for graphics, physics, and simulation
  **GPU 計算** - 圖形、物理與模擬的平行計算
- **AI Inference** - Neural network execution and tensor operations
  **AI 推論** - 神經網路執行與張量運算
- **Massive Parallel Simulation** - 100K+ agent simulation on GPU
  **大規模平行模擬** - GPU 上 10 萬+ 代理模擬
- **Tensor Computation** - Matrix operations and embeddings
  **張量計算** - 矩陣運算與嵌入
- **Neural Processing** - Agent decision making and learning
  **神經處理** - 代理決策與學習
- **Procedural Generation** - World, terrain, and content generation
  **程序化生成** - 世界、地形與內容生成
- **Physics** - Rigid body, fluid, and destruction simulation
  **物理** - 剛體、流體與毀滅模擬
- **Agent Simulation** - AI agent state updates
  **代理模擬** - AI 代理狀態更新
- **World Simulation** - Environment and ecosystem updates
  **世界模擬** - 環境與生態系統更新
- **Quantum-Inspired Algorithms** - Probabilistic optimization and search
  **量子啟發演算法** - 機率優化與搜尋
- **Future Quantum Hardware Abstraction** - Ready for quantum accelerators
  **未來量子硬體抽象** - 準備好量子加速器

### Current Status | 目前狀態

**Status: Experimental / Research**
**狀態：實驗 / 研究**

IQGPU is currently in the architectural design phase. No quantum hardware is claimed or required. StonePotatoes focuses on **quantum-inspired computing** — algorithms and techniques inspired by quantum mechanics (superposition, probabilistic state spaces, annealing-inspired optimization) running on classical GPU hardware.

IQGPU 目前處於架構設計階段。不聲稱也不需要量子硬體。StonePotatoes 專注於 **量子啟發計算** — 在傳統 GPU 硬體上運行的、受量子力學啟發的演算法與技術（疊加、機率狀態空間、退火啟發優化）。

### Future Compute Abstraction | 未來計算抽象

```
CPU
 │
 ├── GPU
 │
 ├── NPU (Neural Processing Unit)
 │
 └── Quantum Accelerator (Future)
```

---

## Core Design Principles | 核心設計原則

### AI Native | AI 原生
AI is not an add-on. AI is a first-class citizen of the engine. Every significant NPC is an autonomous agent with:

AI 不是附加功能。AI 是引擎的一級公民。每個重要的 NPC 都是一個自主代理，具有：

- Identity, Memory, Personality, Values, Needs, Desires, Fears, Beliefs
  身份、記憶、個性、價值觀、需求、慾望、恐懼、信念
- Relationships, Resources, Intelligence, Knowledge, Misconceptions
  關係、資源、智慧、知識、誤解
- Goals, Self-awareness, Models of others and the world
  目標、自我意識、對他人與世界的模型

### GPU First | GPU 優先
Large-scale parallel computation prioritizes:

大規模平行計算優先考慮：

- GPU compute shaders
  GPU 計算著色器
- GPGPU programming
  GPGPU 程式設計
- GPU memory optimization
  GPU 記憶體優化
- Batch processing
  批次處理
- GPU-accelerated physics, AI, and simulation
  GPU 加速物理、AI 與模擬

### Data Oriented | 資料導向
Architecture designed for performance:

為效能而設計的架構：

- ECS (Entity-Component-System)
  ECS（實體-組件-系統）
- Structure of Arrays (SoA)
  陣列結構 (SoA)
- Cache-friendly data layout
  快取友善的資料佈局
- SIMD optimization
  SIMD 優化
- Parallel systems
  平行系統

### Massive Simulation | 大規模模擬
Designed from day one for scale:

從第一天就為規模而設計：

- **Target:** 10,000 agents → 100,000 agents → 1,000,000+ agents
  **目標：** 10,000 代理 → 100,000 代理 → 1,000,000+ 代理
- Hierarchical simulation (LOD systems)
  階層式模擬（LOD 系統）
- Spatial partitioning
  空間分割
- Importance-based scheduling
  基於重要性的排程
- Sleep/wake systems
  休眠/喚醒系統

### Emergent World | 湧現世界
The world is not a script. The world emerges from:

世界不是劇本。世界由以下元素湧現：

- Rules (physics, economics, social systems)
  規則（物理、經濟、社會系統）
- Agents (autonomous decision-making)
  代理（自主決策）
- Resources (scarce, tradable, consumable)
  資源（稀缺、可交易、可消耗）
- Environment (geography, climate, ecosystem)
  環境（地理、氣候、生態系統）
- Time (multi-scale simulation)
  時間（多尺度模擬）

### Deterministic When Needed | 需要時可確定性
Critical simulation supports:

關鍵模擬支援：

- Determinism
  確定性
- Reproducibility
  可重現性
- Replay
  重播
- Debugging
  除錯

### Modular | 模組化
Every core system can be:

每個核心系統都可以：

- Replaced
  替換
- Extended
  擴展
- Plugin-based
  基於外掛
- Tested independently
  獨立測試
- Benchmarked
  基準測試

---

## Core Systems | 核心系統

```
StonePotatoes Engine
│
├── Core (Platform, Memory, Time, Config, Logging, Plugin)
│   核心（平台、記憶體、時間、配置、日誌、外掛）
├── ECS (Entity, Component, System, World, Archetype, Query, Events, Resources)
│   ECS（實體、組件、系統、世界、原型、查詢、事件、資源）
├── Memory (Memory Pool, Allocators, Cache Management)
│   記憶體（記憶體池、分配器、快取管理）
├── Task System (Job scheduling, parallel execution)
│   任務系統（工作排程、平行執行）
├── Rendering (RHI, PBR, HDR, GI, Ray Tracing, Post-Processing)
│   渲染（RHI、PBR、HDR、GI、光線追蹤、後處理）
├── GPU Compute (Compute shaders, GPU memory, kernel scheduling)
│   GPU 計算（計算著色器、GPU 記憶體、核心排程）
├── IQGPU Runtime (Unified compute abstraction, compute graph)
│   IQGPU 運行時（統一計算抽象、計算圖）
├── Physics (Rigid Body, Soft Body, Fluid, Destruction, Vehicle, Character)
│   物理（剛體、軟體、流體、毀滅、載具、角色）
├── AI Runtime (Agent Runtime, Memory, Decision Making, Perception, Behavior)
│   AI 運行時（代理運行時、記憶體、決策、感知、行為）
├── Agent System (NPC Generator, AI Director, Narrative Engine, Quest System)
│   代理系統（NPC 生成器、AI 導演、敘事引擎、任務系統）
├── World Simulation (Time, Geography, Climate, Ecosystem, Procedural Generation)
│   世界模擬（時間、地理、氣候、生態系統、程序化生成）
├── Civilization Simulation (Politics, Economy, Military, Technology, Culture)
│   文明模擬（政治、經濟、軍事、科技、文化）
├── Navigation (Pathfinding, Flow Fields, Steering Behaviors)
│   導航（路徑規劃、流場、操控行為）
├── Networking (Server, Client, Replication, RPC, Cloud Simulation)
│   網路（伺服器、客戶端、複製、RPC、雲端模擬）
├── Audio (Sound, Music, Spatial Audio)
│   音訊（音效、音樂、空間音訊）
├── Asset Pipeline (Import, Processing, Cooking, Streaming)
│   資產管線（匯入、處理、編譯、串流）
├── Editor (World Editor, Civilization Editor, AI Debugger, Profiler)
│   編輯器（世界編輯器、文明編輯器、AI 除錯器、分析器）
├── Scripting (Lua, Python, TypeScript)
│   腳本（Lua、Python、TypeScript）
├── Modding (Mod Loader, Mod API, Sandbox, Hot-reload)
│   Modding（Mod 載入器、Mod API、沙盒、熱重載）
├── Debugging (GPU Profiler, AI Debugger, World Inspector, Timeline)
│   除錯（GPU 分析器、AI 除錯器、世界檢查器、時間軸）
└── Developer Tools (Benchmark, Profiler, Console, Asset Browser)
    開發者工具（基準測試、分析器、主控台、資產瀏覽器）
```

---

## AI Agent Architecture | AI 代理架構

StonePotatoes does not use simple NPC scripts. It uses an **Autonomous Agent Architecture**.

StonePotatoes 不使用簡單的 NPC 腳本。它使用 **自主代理架構**。

### Agent Capabilities | 代理能力

Each agent can have:

每個代理可以具有：

- Identity
  身份
- Goals (long-term, short-term, immediate)
  目標（長期、短期、立即）
- Needs (physiological, safety, social, esteem, self-actualization)
  需求（生理、安全、社交、尊重、自我實現）
- Memory (short-term, episodic, semantic, social, trauma)
  記憶（短期、情節、語意、社交、創傷）
- Knowledge (world facts, misconceptions)
  知識（世界事實、誤解）
- Beliefs (values, biases, ideologies)
  信念（價值觀、偏見、意識形態）
- Relationships (trust, fear, respect, love, hatred)
  關係（信任、恐懼、尊重、愛、仇恨）
- Personality (OCEAN traits, values, preferences)
  個性（OCEAN 特質、價值觀、偏好）
- Resources (inventory, money, social capital)
  資源（庫存、金錢、社會資本）
- Plans (strategy, tactics, contingencies)
  計畫（策略、戰術、應變）
- Decision Making (utility-based, planning, review)
  決策（基於效用、規劃、審查）

### AI Hierarchy | AI 階層

```
Layer 5: Civilization (Faction-level strategy, diplomacy, war)
第五層：文明（派系級策略、外交、戰爭）
    ↓
Layer 4: World (Global events, history, world state)
第四層：世界（全球事件、歷史、世界狀態）
    ↓
Layer 3: Strategic (Long-term goals, life planning)
第三層：策略（長期目標、人生規劃）
    ↓
Layer 2: Operational (Resource management, daily routines)
第二層：作戰（資源管理、日常例行）
    ↓
Layer 1: Tactical (Combat, immediate threats, squad coordination)
第一層：戰術（戰鬥、立即威脅、小隊協調）
    ↓
Layer 0: Reflex (Immediate reactions, instincts)
第零層：反射（立即反應、本能）
```

---

## World Simulation | 世界模擬

The world is a **Living World** that includes:

世界是一個 **活著的世界**，包含：

- Geography (terrain, rivers, mountains, resources)
  地理（地形、河流、山脈、資源）
- Weather (climate, seasons, storms, temperature)
  天氣（氣候、季節、風暴、溫度）
- Climate (long-term climate change, temperature zones)
  氣候（長期氣候變遷、溫度帶）
- Ecosystem (plants, animals, food chains, population dynamics)
  生態系統（植物、動物、食物鏈、人口動態）
- Resources (extraction, transportation, consumption, scarcity)
  資源（開採、運輸、消耗、稀缺）
- Population (demographics, migration, social classes)
  人口（人口統計、遷移、社會階級）
- Cities (infrastructure, economy, governance)
  城市（基礎設施、經濟、治理）
- Economy (trade, markets, production, currency, inflation)
  經濟（貿易、市場、生產、貨幣、通貨膨脹）
- Politics (factions, governments, laws, policies)
  政治（派系、政府、法律、政策）
- Culture (religion, art, values, traditions)
  文化（宗教、藝術、價值觀、傳統）
- Technology (research, innovation, diffusion)
  科技（研究、創新、擴散）
- Warfare (armies, logistics, battles, fortifications)
  戰爭（軍隊、後勤、戰役、防禦工事）
- Diplomacy (relations, treaties, alliances, federations)
  外交（關係、條約、同盟、聯邦）

**The world continues to run even when the player is not present.**
**即使玩家不在場，世界仍繼續運行。**

---

## Civilization Simulation | 文明模擬

The current C++ slice provides `StonePotatoes::Civilization::Economic::SDModel`,
a discrete stock-flow simulator with stocks, parameters, auxiliary values,
flows, scheduled events, probes, and history. It is available to external CMake
projects through the installed `StonePotatoes::Civilization` target. Population,
trade, politics, culture, and other complete civilization systems remain
long-term goals:

目前 C++ 切片提供 `StonePotatoes::Civilization::Economic::SDModel`，這是離散的
存量／流量模擬器，支援存量、參數、輔助值、流量、排程事件、探針與歷史記錄。
人口、貿易、政治、文化等完整文明系統仍屬長期目標：

- Population (demographics, birth/death rates, migration)
  人口（人口統計、出生/死亡率、遷移）
- Economy (trade, markets, production, taxation, currency)
  經濟（貿易、市場、生產、稅收、貨幣）
- Technology (research, innovation, tech trees)
  科技（研究、創新、科技樹）
- Culture (religion, art, values, language, influence)
  文化（宗教、藝術、價值觀、語言、影響力）
- Politics (government types, factions, policies, laws)
  政治（政府類型、派系、政策、法律）
- Military (armies, units, logistics, warfare)
  軍事（軍隊、單位、後勤、戰爭）
- Diplomacy (relations, treaties, alliances, wars)
  外交（關係、條約、同盟、戰爭）

### Civilization Dynamics | 文明動態

Civilizations can:

文明可以：

- Build (establish identity, legitimacy, infrastructure)
  建立（建立身份、合法性、基礎設施）
- Expand (acceleration, mobilization, conquest)
  擴張（加速、動員、征服）
- Stabilize (governance, resource integration, institutions)
  穩定（治理、資源整合、制度）
- Transform (revolution, reform, political change)
  轉型（革命、改革、政治變革）
- Decline (economic collapse, military defeat, social unrest)
  衰退（經濟崩潰、軍事失敗、社會動盪）
- Fall (destruction, fragmentation, collapse)
  沒落（毀滅、分裂、崩潰）
- Revive (reconstruction, renaissance, restoration)
  復興（重建、文藝復興、恢復）

---

## Emergent Narrative | 湧現敘事

StonePotatoes features an **Emergent Narrative Engine**. Stories are not fixed scripts. Events emerge from:

StonePotatoes 具備 **湧現敘事引擎**。故事不是固定劇本。事件由以下元素湧現：

- Agent Actions (decisions, plans, consequences)
  代理行動（決策、計畫、後果）
- World State (resources, geography, environment)
  世界狀態（資源、地理、環境）
- History (past events, world chronicle)
  歷史（過去事件、世界編年史）
- Relationships (trust, hatred, alliances)
  關係（信任、仇恨、同盟）
- Resources (scarcity, abundance, distribution)
  資源（稀缺、豐富、分配）
- Randomness (unexpected events, black swans)
  隨機性（意外事件、黑天鵝）

### Example: Emergent History | 範例：湧現歷史

1. NPC A saves NPC B from danger
   NPC A 從危險中救下 NPC B
2. B becomes a political figure due to reputation
   B 因聲譽成為政治人物
3. B changes city policy based on beliefs
   B 根據信念改變城市政策
4. Diplomatic relations deteriorate with neighboring city
   與鄰近城市的外交關係惡化
5. War breaks out
   戰爭爆發
6. City is destroyed
   城市被毀
7. Survivors rebuild with new government
   倖存者以新政府重建
8. New civilization forms with different values
   新文明以不同價值觀形成

This creates **Machine Generated History** — unique, replayable, emergent narratives.
這創造了 **機器生成歷史** — 獨特、可重玩、湧現的敘事。

---

## GPU Architecture | GPU 架構

StonePotatoes treats the GPU not just as a rendering device, but as a **General Simulation Processor**.

StonePotatoes 不僅將 GPU 視為渲染裝置，而是視為 **通用模擬處理器**。

### GPU Compute Architecture | GPU 計算架構

- GPU Compute (compute shaders, GPU kernels)
  GPU 計算（計算著色器、GPU 核心）
- Compute Shader (shader-based computation)
  計算著色器（基於著色器的計算）
- GPU Buffers (structured buffers, UAVs, SRVs)
  GPU 緩衝區（結構化緩衝區、UAV、SRV）
- GPU Memory (managed memory, zero-copy)
  GPU 記憶體（受管記憶體、零複製）
- Async Compute (parallel compute and graphics)
  非同步計算（平行計算與圖形）
- Multi-GPU (distributed simulation across GPUs)
  多 GPU（跨 GPU 分散式模擬）
- GPU Simulation (physics, particles, crowd, AI)
  GPU 模擬（物理、粒子、群眾、AI）
- GPU Physics (rigid body, fluid, destruction)
  GPU 物理（剛體、流體、毀滅）
- GPU AI (neural network inference, batch evaluation)
  GPU AI（神經網路推論、批次評估）

### GPU is the Brain | GPU 是大腦

In StonePotatoes, the GPU is responsible for:

在 StonePotatoes 中，GPU 負責：

- Rendering (graphics, lighting, shadows)
  渲染（圖形、光照、陰影）
- Physics (collision, dynamics, constraints)
  物理（碰撞、動力學、約束）
- Particle systems (effects, explosions, fluids)
  粒子系統（特效、爆炸、流體）
- Crowd simulation (pathfinding, behavior)
  群眾模擬（路徑規劃、行為）
- World simulation (ecosystem, weather)
  世界模擬（生態系統、天氣）
- AI inference (neural networks, decision making)
  AI 推論（神經網路、決策）
- Pathfinding (A*, flow fields, navigation)
  路徑規劃（A*、流場、導航）
- Visibility computation (line of sight, occlusion)
  可見性計算（視線、遮擋）
- Procedural generation (terrain, cities, content)
  程序化生成（地形、城市、內容）

---

## IQGPU Runtime | IQGPU 運行時

```
Application
     │
     ▼
StonePotatoes Runtime
     │
     ▼
    IQGPU
     │
 ┌───┼────────────┐
 ▼   ▼            ▼
CPU GPU       Accelerator
     │
     ▼
Compute Graph
     │
     ▼
Simulation
```

### Compute Graph | 計算圖

StonePotatoes uses a **Compute Graph** to represent:

StonePotatoes 使用 **計算圖** 來表示：

- AI computation
  AI 計算
- Physics simulation
  物理模擬
- Rendering pipeline
  渲染管線
- World simulation
  世界模擬
- Procedural generation
  程序化生成

As computational nodes in a directed acyclic graph (DAG).
作為有向無環圖 (DAG) 中的計算節點。

```
World State
     │
     ├── AI Compute Node
     │   AI 計算節點
     ├── Physics Compute Node
     │   物理計算節點
     ├── Economy Compute Node
     │   經濟計算節點
     └── Weather Compute Node
         天氣計算節點
          │
          ▼
     World Update Node
     世界更新節點
          │
          ▼
     Rendering Node
     渲染節點
```

### Future Support | 未來支援

The compute graph is designed to support:

計算圖設計為支援：

- CPU Nodes (complex control flow, logic)
  CPU 節點（複雜控制流、邏輯）
- GPU Nodes (parallel computation, simulation)
  GPU 節點（平行計算、模擬）
- NPU Nodes (neural network inference)
  NPU 節點（神經網路推論）
- Quantum Nodes (future quantum accelerators)
  量子節點（未來量子加速器）

---

## Multi-Resolution Simulation | 多解析度模擬

To achieve massive scale, StonePotatoes uses **Multi-Resolution Simulation** (LOD for simulation fidelity).

為了達到大規模，StonePotatoes 使用 **多解析度模擬**（模擬保真度的 LOD）。

### Simulation LOD Levels | 模擬 LOD 層級

- **Level 0:** Full Simulation (near player, high-fidelity AI, full physics)
  **第 0 層：** 完整模擬（玩家附近、高保真 AI、完整物理）
- **Level 1:** High Fidelity (medium distance, reduced AI frequency)
  **第 1 層：** 高保真（中等距離、降低 AI 頻率）
- **Level 2:** Medium Fidelity (far distance, abstracted AI)
  **第 2 層：** 中保真（遠距離、抽象化 AI）
- **Level 3:** Low Fidelity (very far, statistical models)
  **第 3 層：** 低保真（非常遠、統計模型）
- **Level 4:** Sleep (inactive entities, no computation)
  **第 4 層：** 休眠（非活動實體、無計算）

Player nearby: High precision simulation.
Player far away: Statistical models to avoid expensive AI computation.
玩家附近：高精度模擬。
玩家遠處：統計模型以避免昂貴的 AI 計算。

---

## World Time | 世界時間

StonePotatoes supports **Multi-Scale Time**:

StonePotatoes 支援 **多尺度時間**：

```
Frame (60 FPS)
  幀（60 FPS）
  ↓
Tick (game logic update)
  Tick（遊戲邏輯更新）
  ↓
Second (NPC activities)
  秒（NPC 活動）
  ↓
Minute (movement, short-term actions)
  分（移動、短期行動）
  ↓
Hour (economy, cities)
  時（經濟、城市）
  ↓
Day (population, society)
  日（人口、社會）
  ↓
Month (politics, markets)
  月（政治、市場）
  ↓
Year (civilization, technology)
  年（文明、科技）
  ↓
Era (civilization evolution)
  紀元（文明演化）
```

Time control supports:

時間控制支援：

- Pause
  暫停
- Resume
  繼續
- Fast Forward
  快進
- Replay
  重播
- Deterministic Simulation
  確定性模擬

---

## World History | 世界歷史

StonePotatoes includes a **World Chronicle** system that automatically records:

StonePotatoes 包含 **世界編年史** 系統，自動記錄：

- Wars and battles
  戰爭與戰役
- Kings and leaders
  國王與領袖
- Cities founded and destroyed
  城市的建立與毀滅
- Revolutions and reforms
  革命與改革
- Discoveries and inventions
  發現與發明
- Disasters and plagues
  災難與瘟疫
- Alliances and treaties
  同盟與條約
- Civilizations rise and fall
  文明的興衰
- Major characters and their deeds
  主要人物及其事蹟

This forms **Machine Generated History** — a persistent, queryable timeline of world events.
這形成了 **機器生成歷史** — 一個持久、可查詢的世界事件時間軸。

---

## Rendering | 渲染

The engine now includes a foundational 2D renderer with Vulkan presentation and
ImGui, plus an OpenGL fallback for CPU-only builds. The following advanced
rendering features remain planned:

引擎目前已有 2D 渲染基礎，支援 Vulkan 呈現與 ImGui，CPU-only 建置則保留
OpenGL 後備。以下進階渲染功能仍在規劃中：

- PBR (Physically Based Rendering)
  PBR（基於物理的渲染）
- HDR (High Dynamic Range)
  HDR（高動態範圍）
- Global Illumination (screen-space, voxel-based)
  全域光照（螢幕空間、體素基礎）
- Ray Tracing (reflections, shadows, global illumination)
  光線追蹤（反射、陰影、全域光照）
- Path Tracing (progressive path tracing)
  路徑追蹤（漸進式路徑追蹤）
- Virtual Geometry (Nanite-like streaming)
  虛擬幾何（Nanite 類串流）
- Virtual Texturing (sparse texture streaming)
  虛擬紋理（稀疏紋理串流）
- Volumetric Effects (fog, clouds, lighting)
  體積特效（霧、雲、光照）
- Terrain (height-based, procedural)
  地形（高度基礎、程序化）
- Ocean (water simulation, waves)
  海洋（水模擬、波浪）
- Clouds (volumetric, dynamic)
  雲（體積、動態）
- Destruction (building destruction, terrain modification)
  毀滅（建築毀滅、地形修改）
- Hair (hair simulation, rendering)
  髮型（髮型模擬、渲染）
- Skin (subsurface scattering)
  皮膚（次表面散射）
- Massive Crowd (thousands of characters)
  大規模群眾（數千個角色）

**Status: The foundational 2D rendering slice is implemented; Phase 3 remains in progress for advanced features.**
**狀態：2D 渲染基礎切片已完成；第三階段尚未完成，進階功能列為後續工作。**

---

## Physics | 物理

The engine includes a fixed-tick 2D rigid-body and axis-aligned box-contact
foundation. It is available to external CMake projects as the installed
`StonePotatoes::Physics` target; the installed-package consumer verifies
gravity/force integration, high-speed wall contact, and friction through the
public API. Additional physics systems remain planned:

引擎已有固定 tick 的 2D 剛體與軸對齊方盒接觸基礎；安裝後 consumer 亦透過公開 API 驗收高速撞牆與摩擦，其他物理系統仍在規劃中：

- Advanced Rigid Body (rotated shapes, joints, and richer dynamics)
  進階剛體（旋轉形狀、關節與更完整的動力學）
- Soft Body (deformation, cloth)
  軟體（變形、布料）
- Fluid (water, gases)
  流體（水、氣體）
- Destruction (building destruction, terrain)
  毀滅（建築毀滅、地形）
- Vehicle (vehicle simulation)
  載具（載具模擬）
- Character (character controller)
  角色（角色控制器）
- Particle (particle simulation)
  粒子（粒子模擬）
- Rope (rope and chain simulation)
  繩索（繩索與鏈條模擬）

**Future Goal:** GPU Accelerated Physics (Phase 8)
**未來目標：** GPU 加速物理（第八階段）

**Status: Phase 4 in progress; advanced systems planned**
**狀態：第四階段進行中；進階系統仍在規劃**

---

## Massive Agent Simulation (Target) | 大規模代理模擬（目標）

StonePotatoes has the following long-term performance targets:

StonePotatoes 具有以下長期效能目標：

- **Phase 6:** 10,000 agents with full AI
  **第六階段：** 10,000 個具備完整 AI 的代理
- **Phase 8:** 100,000 agents with full AI
  **第八階段：** 100,000 個具備完整 AI 的代理
- **Phase 8:** 1,000,000 agents (abstracted simulation)
  **第八階段：** 1,000,000 個代理（抽象化模擬）
- **Future:** 10,000,000+ simulated entities
  **未來：** 10,000,000+ 模擬實體

Techniques to achieve this:

達成此目標的技術：

- ECS (data-oriented design)
  ECS（資料導向設計）
- GPU Compute (parallel simulation)
  GPU 計算（平行模擬）
- LOD Simulation (multi-resolution fidelity)
  LOD 模擬（多解析度保真度）
- Hierarchical Simulation (world → region → entity)
  階層式模擬（世界 → 區域 → 實體）
- Spatial Partition (grid, octree, BVH)
  空間分割（網格、八叉樹、BVH）
- Batch Processing (GPU batch updates)
  批次處理（GPU 批次更新）
- Importance Scheduling (prioritize important agents)
  重要性排程（優先處理重要代理）
- Sleep/Wake (inactive entities don't compute)
  休眠/喚醒（非活動實體不計算）
- Event Driven Simulation (only update on events)
  事件驅動模擬（僅在事件時更新）

**Note: These are design targets. Actual performance will be measured during development.**
**註：這些是設計目標。實際效能將在開發過程中測量。**

---

## Developer Experience (Planned) | 開發者體驗（計畫中）

StonePotatoes aims to provide world-class developer tools:

StonePotatoes 旨在提供世界級的開發者工具：

- **Editor** (World editor, civilization editor, NPC designer)
  **編輯器**（世界編輯器、文明編輯器、NPC 設計器）
- **Inspector** (World inspector, component inspector)
  **檢查器**（世界檢查器、組件檢查器）
- **Debugger** (GPU profiler, AI debugger, timeline, replay)
  **除錯器**（GPU 分析器、AI 除錯器、時間軸、重播）
- **Profiler** (Performance profiling, optimization guidance)
  **分析器**（效能分析、優化指導）
- **Agent Inspector** (View agent goals, needs, memory, relationships, strategy)
  **代理檢查器**（查看代理目標、需求、記憶、關係、策略）
- **World Inspector** (View world state, history, civilization data)
  **世界檢查器**（查看世界狀態、歷史、文明資料）
- **Timeline** (History visualization, time control, replay)
  **時間軸**（歷史視覺化、時間控制、重播）
- **Benchmark** (Performance benchmarking, comparison)
  **基準測試**（效能基準測試、比較）
- **Console** (In-game console, command system)
  **主控台**（遊戲內主控台、指令系統）

### Agent Inspector | 代理檢查器

Developers can inspect an agent's internal state:

開發者可以檢查代理的內部狀態：

```
Agent
 ├── Identity (name, faction, occupation)
 │   身份（姓名、派系、職業）
 ├── Goals (current goals, priorities)
 │   目標（當前目標、優先級）
 ├── Needs (hunger, thirst, safety, social)
 │   需求（飢餓、口渴、安全、社交）
 ├── Memory (short-term, episodic, semantic)
 │   記憶（短期、情節、語意）
 ├── Knowledge (world facts, misconceptions)
 │   知識（世界事實、誤解）
 ├── Beliefs (values, biases, ideologies)
 │   信念（價值觀、偏見、意識形態）
 ├── Relationships (trust, fear, respect, love, hatred)
 │   關係（信任、恐懼、尊重、愛、仇恨）
 ├── Personality (OCEAN traits, values)
 │   個性（OCEAN 特質、價值觀）
 ├── Resources (inventory, money, social capital)
 │   資源（庫存、金錢、社會資本）
 ├── Current Strategy (plan, tactics, contingencies)
 │   當前策略（計畫、戰術、應變）
 ├── Decision Score (utility scores, action evaluation)
 │   決策分數（效用分數、行動評估）
 └── Next Action (selected action, reasoning)
     下一行動（選定的行動、推理）
```

---

## Repository Structure | 儲存庫結構

```
StonePotatoes/
│
├── README.md                    # This file
│   # 本檔案
├── LICENSE                      # MIT License
│   # MIT 授權
├── CMakeLists.txt               # Main CMake build file
│   # 主要 CMake 建置檔案
├── ARCHITECTURE.md              # Complete engine architecture
│   # 完整引擎架構
├── ROADMAP.md                   # Technical roadmap (Phase 0-13)
│   # 技術路線圖（第 0-13 階段）
├── REPOSITORY_STRUCTURE.md      # Repository structure explanation
│   # 儲存庫結構說明
│
├── src/                         # Source code
│   # 原始碼
│   ├── core/                    # Core engine systems ✓
│   # 核心引擎系統 ✓
│   ├── ecs/                     # Entity-Component-System ✓
│   # 實體-組件-系統 ✓
│   ├── rendering/               # Vulkan renderer + ImGui/2D; OpenGL fallback for CPU-only builds
│   # Vulkan 渲染器 + ImGui/2D；CPU-only 建置保留 OpenGL 後備
│   ├── physics/                 # Fixed-tick 2D bodies and AABB contacts
│   # 固定 tick 2D 剛體與 AABB 接觸
│   ├── ai/                      # AI engine (stub)
│   # AI 引擎（存根）
│   ├── world/                   # World simulation (stub)
│   # 世界模擬（存根）
│   ├── civilization/            # Economic stock-flow simulation (SystemDynamics)
│   # 文明經濟存量／流量模擬（SystemDynamics）
│   ├── agent/                   # Agent systems (stub)
│   # 代理系統（存根）
│   ├── networking/              # Deterministic lockstep commands (partial)
│   # 決定性 lockstep 指令同步（部分）
│   ├── data/                    # Root-scoped binary/text asset cache
│   # 資料與資產（根目錄限制的二進位／文字快取）
│   ├── gpu/                     # GPU compute (stub)
│   # GPU 計算（存根）
│   ├── game/                    # Game engine API (stub)
│   # 遊戲引擎 API（存根）
│   ├── api/                     # Developer APIs (stub)
│   # 開發者 API（存根）
│   │   ├── cpp/
│   │   ├── python/
│   │   ├── lua/
│   │   └── rust/
│   └── main.cpp                 # Main executable ✓
│       # 主執行檔 ✓
│
├── tests/                       # Tests (stub)
│   # 測試（存根）
├── examples/                    # Example projects (stub)
│   # 範例專案（存根）
├── tools/                       # Build tools and scripts (stub)
│   # 建置工具與腳本（存根）
├── assets/                      # Engine assets (stub)
│   # 引擎資產（存根）
├── external/                    # Third-party dependencies (stub)
│   # 第三方相依項（存根）
└── build/                       # Build output (generated, gitignored)
    # 建置輸出（生成、gitignored）
```

**Note:** Directories marked with ✓ contain initial implementations. Directories marked with (stub) are architectural placeholders.
**註：** 標有 ✓ 的目錄包含初始實作。標有 (stub) 的目錄是架構佔位符。

---

## Technology Stack | 技術堆疊

### Currently Implemented | 目前已實作

- **Language:** C++20
  **語言：** C++20
- **Build System:** CMake 3.20+
  **建置系統：** CMake 3.20+
- **Platform:** Windows, Linux, macOS (planned)
  **平台：** Windows、Linux、macOS（計畫中）
- **Compiler:** MSVC 2019+, GCC 10+, Clang 10+
  **編譯器：** MSVC 2019+、GCC 10+、Clang 10+

### Candidate Stack (Planned) | 候選堆疊（計畫中）

- **Rendering APIs:** DirectX 12, Vulkan, Metal
  **渲染 API：** DirectX 12、Vulkan、Metal
- **GPU Compute:** CUDA, HIP, OpenCL, WebGPU
  **GPU 計算：** CUDA、HIP、OpenCL、WebGPU
- **AI Frameworks:** TensorFlow, PyTorch, ONNX Runtime
  **AI 框架：** TensorFlow、PyTorch、ONNX Runtime
- **Scripting:** Lua, Python, TypeScript
  **腳本：** Lua、Python、TypeScript
- **Math:** GLM, Eigen
  **數學：** GLM、Eigen
- **Serialization:** JSON, YAML, Protocol Buffers
  **序列化：** JSON、YAML、Protocol Buffers

**Actual implementation depends on platform and project phase.**
**實際實作取決於平台與專案階段。**

---

## Build Instructions | 建置說明

### Prerequisites | 前置需求

- CMake 3.20 or higher
  CMake 3.20 或更高版本
- C++20 compatible compiler (MSVC 2019+, GCC 10+, Clang 10+)
  C++20 相容編譯器（MSVC 2019+、GCC 10+、Clang 10+）
- Git
  Git

### Build Steps | 建置步驟

```bash
# Clone the repository
# 複製儲存庫
git clone https://github.com/yourusername/StonePotatoes.git
cd StonePotatoes

# Configure with CMake
# 使用 CMake 配置
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build
# 建置
cmake --build build --config Release

# Run (Visual Studio multi-config)
# 執行（Visual Studio 多組態）
./build/bin/Release/StonePotatoes.exe

# Create a platform package in build/packages/
# 在 build/packages/ 建立平台發行套件
cpack --config build/CPackConfig.cmake -C Release
```

Single-config generators place the executable at `build/bin/StonePotatoes`; Visual Studio places it at `build/bin/Release/StonePotatoes.exe`.
單組態產生器會將執行檔放在 `build/bin/StonePotatoes`；Visual Studio 則放在 `build/bin/Release/StonePotatoes.exe`。

---

## Quick Start | 快速開始

**Current Status:** StonePotatoes is in active early development (Phase 1). No playable demo is available yet.
**目前狀態：** StonePotatoes 處於活躍的早期開發階段（第一階段）。尚無可玩的展示。

To contribute or experiment:

若要貢獻或實驗：

1. Clone the repository
   複製儲存庫
2. Build using CMake (see Build Instructions)
   使用 CMake 建置（參閱建置說明）
3. Run the main executable (creates window, initializes ECS)
   執行主執行檔（建立視窗、初始化 ECS）
4. Explore the source code in `src/`
   探索 `src/` 中的原始碼
5. Read the architecture documentation
   閱讀架構文件

---

## Roadmap | 路線圖

See [ROADMAP.md](ROADMAP.md) for the complete technical roadmap.
完整技術路線圖請參閱 [ROADMAP.md](ROADMAP.md)。

### High-Level Phases | 高階階段

- **Phase 0 — Architecture** ✓ (Complete)
  **第 0 階段 — 架構** ✓（完成）
- **Phase 1 — Core Engine** 🚧 (In Progress)
  **第 1 階段 — 核心引擎** 🚧（進行中）
- **Phase 2 — ECS** 📋 (Planned)
  **第 2 階段 — ECS** 📋（計畫中）
- **Phase 3 — Rendering** 🚧 (In Progress)
  **第 3 階段 — 渲染** 🚧（進行中）
- **Phase 4 — Physics** 🚧 (In Progress)
  **第 4 階段 — 物理** 🚧（進行中）
- **Phase 5 — AI Agent** 📋 (Planned)
  **第 5 階段 — AI 代理** 📋（計畫中）
- **Phase 6 — World Simulation** 📋 (Planned)
  **第 6 階段 — 世界模擬** 📋（計畫中）
- **Phase 7 — Civilization Simulation** 📋 (Planned)
  **第 7 階段 — 文明模擬** 📋（計畫中）
- **Phase 8 — GPU Massive Simulation** 📋 (Planned)
  **第 8 階段 — GPU 大規模模擬** 📋（計畫中）
- **Phase 9 — Networking** 📋 (Planned)
  **第 9 階段 — 網路** 📋（計畫中）
- **Phase 10 — Editor** 🧪 (The first entity hierarchy and identity inspector slice is implemented; broader editing tools remain planned)
  **第 10 階段 — 編輯器** 🧪（首個實體階層與 identity inspector 切片已完成；完整編輯工具仍在規劃中）
- **Phase 11 — Mod System** 📋 (Planned)
  **第 11 階段 — Mod 系統** 📋（計畫中）
- **Phase 12 — Cloud/Distributed Simulation** 📋 (Planned)
  **第 12 階段 — 雲端/分散式模擬** 📋（計畫中）
- **Phase 13 — AAA Production** 📋 (Planned)
  **第 13 階段 — AAA 量產** 📋（計畫中）

---

## Performance Goals | 效能目標

### Design Targets | 設計目標

- **Scale:** 1,000,000 simulated agents (long-term target)
  **規模：** 1,000,000 個模擬代理（長期目標）
- **FPS:** 60 FPS at 1080p (with 10,000 agents)
  **FPS：** 1080p 下 60 FPS（含 10,000 個代理）
- **Determinism:** Reproducible simulation results
  **確定性：** 可重現的模擬結果
- **Memory:** Efficient memory usage with ECS
  **記憶體：** 使用 ECS 的高效記憶體使用

### Measured Performance | 測量效能

**Not yet benchmarked.** Performance will be measured during development phases.
**尚未基準測試。** 效能將在開發階段中測量。

---

## Testing (Planned) | 測試（計畫中）

StonePotatoes will include comprehensive testing:

StonePotatoes 將包含全面測試：

- Unit Tests (component-level testing)
  單元測試（組件級測試）
- Integration Tests (system-level testing)
  整合測試（系統級測試）
- ECS Tests (entity, component, system tests)
  ECS 測試（實體、組件、系統測試）
- GPU Tests (compute shader tests)
  GPU 測試（計算著色器測試）
- AI Tests (agent behavior tests)
  AI 測試（代理行為測試）
- Simulation Tests (world, civilization tests)
  模擬測試（世界、文明測試）
- Determinism Tests (reproducibility validation)
  確定性測試（可重現性驗證）
- Network Tests (multiplayer validation)
  網路測試（多人遊戲驗證）
- Stress Tests (scale testing)
  壓力測試（規模測試）
- Benchmark Tests (performance measurement)
  基準測試（效能測量）

---

## Security / Stability | 安全性 / 穩定性

StonePotatoes prioritizes:

StonePotatoes 優先考慮：

- Memory Safety (no memory leaks, safe allocators)
  記憶體安全（無記憶體洩漏、安全分配器）
- Input Validation (validate all external inputs)
  輸入驗證（驗證所有外部輸入）
- Plugin Isolation (sandbox mods and plugins)
  外掛隔離（沙盒 mod 與外掛）
- Deterministic State Validation (verify simulation integrity)
  確定性狀態驗證（驗證模擬完整性）
- Save Integrity (validate save files)
  存檔完整性（驗證存檔）
- Network Validation (validate network packets)
  網路驗證（驗證網路封包）
- Crash Recovery (graceful error handling)
  當機恢復（優雅錯誤處理）

---

## Modding (Planned) | Modding（計畫中）

StonePotatoes will support extensive modding:

StonePotatoes 將支援廣泛的 modding：

- Gameplay Mods (modify game rules)
  遊戲玩法 Mod（修改遊戲規則）
- World Mods (modify world generation)
  世界 Mod（修改世界生成）
- AI Mods (modify AI behavior)
  AI Mod（修改 AI 行為）
- Civilization Mods (modify civilization systems)
  文明 Mod（修改文明系統）
- Rendering Mods (modify rendering pipeline)
  渲染 Mod（修改渲染管線）
- Simulation Mods (modify simulation parameters)
  模擬 Mod（修改模擬參數）
- Asset Mods (add custom assets)
  資產 Mod（新增自訂資產）

---

## License | 授權

StonePotatoes is licensed under the MIT License. See [LICENSE](LICENSE) file for details.
StonePotatoes 採用 MIT 授權。詳情請參閱 [LICENSE](LICENSE) 檔案。

Copyright (c) 2026 ArchaonHW

---

## Contributing | 貢獻

Contributions are welcome! Please:
歡迎貢獻！請：

1. Read the [ARCHITECTURE.md](ARCHITECTURE.md) and [ROADMAP.md](ROADMAP.md) first
   首先閱讀 [ARCHITECTURE.md](ARCHITECTURE.md) 與 [ROADMAP.md](ROADMAP.md)
2. Focus on the current development phase
   專注於當前開發階段
3. Ensure real implementations — no stubs or fake code
   確保真實實作 — 無存根或假程式碼
4. Follow coding standards (C++20, consistent style)
   遵循編碼標準（C++20、一致風格）
5. Add tests for new features
   為新功能新增測試
6. Document significant changes
   記錄重大變更

### Engineering Principles | 工程原則

- **No Fake Features** — Do not use fake APIs or fake completion states
  **無假功能** — 不使用假 API 或假完成狀態
- **No Premature Claims** — Do not claim performance without measurement
  **無過早聲稱** — 不在測量前聲稱效能
- **No Hidden Complexity** — Document major architectural decisions
  **無隱藏複雜性** — 記錄重大架構決策
- **Benchmark Everything Important** — Measure performance before optimization
  **基準測試所有重要事物** — 優化前測量效能
- **Determinism Matters** — Simulation must be reproducible
  **確定性很重要** — 模擬必須可重現
- **Scale From Day One** — Architecture must consider 1M+ agents
  **從第一天就考慮規模** — 架構必須考慮 1M+ 代理

---

## Documentation | 文件

- [ARCHITECTURE.md](ARCHITECTURE.md) — Complete engine architecture
  [ARCHITECTURE.md](ARCHITECTURE.md) — 完整引擎架構
- [ROADMAP.md](ROADMAP.md) — Technical roadmap (Phase 0-13)
  [ROADMAP.md](ROADMAP.md) — 技術路線圖（第 0-13 階段）
- [REPOSITORY_STRUCTURE.md](REPOSITORY_STRUCTURE.md) — Repository structure explanation
  [REPOSITORY_STRUCTURE.md](REPOSITORY_STRUCTURE.md) — 儲存庫結構說明

---

## Acknowledgments | 致謝

StonePotatoes is designed by a team of:
StonePotatoes 由以下團隊設計：

- Game Engine Architects
  遊戲引擎架構師
- GPU Compute Architects
  GPU 計算架構師
- AI Agent Engineers
  AI 代理工程師
- Game AI Researchers
  遊戲 AI 研究員
- Operating System Engineers
  作業系統工程師
- Graphics Engineers
  圖形工程師
- Physics Engine Engineers
  物理引擎工程師
- Network Architects
  網路架構師
- Database Engineers
  資料庫工程師
- Toolchain Engineers
  工具鏈工程師
- UX Engineers
  UX 工程師
- Game Designers
  遊戲設計師

---

## Core Philosophy | 核心哲學

StonePotatoes is not:
StonePotatoes 不是：

- "Another game engine"
  「另一個遊戲引擎」
- "Just a rendering engine"
  「僅僅是一個渲染引擎」
- "Just an AI system"
  「僅僅是一個 AI 系統」
- "Just a simulation engine"
  「僅僅是一個模擬引擎」

StonePotatoes is:
StonePotatoes 是：

**An intelligent world computation platform.**
**一個智能世界計算平台。**

### Core Concepts | 核心概念

- **Think** — AI agents with memory, goals, and decision-making
  **思考** — 具有記憶、目標與決策能力的 AI 代理
- **Simulate** — Living worlds that evolve over time
  **模擬** — 隨時間演化的活著世界
- **Generate** — Procedural worlds, civilizations, and narratives
  **生成** — 程序化世界、文明與敘事
- **Evolve** — Emergent behavior, history, and consequences
  **演化** — 湧現行為、歷史與後果

### Ultimate Goal | 終極目標

StonePotatoes does not aim to simply make GPUs render prettier graphics.
StonePotatoes 不僅僅旨在讓 GPU 渲染更漂亮的圖形。

**It aims to make GPUs the brain of the world.**
**它旨在讓 GPU 成為世界的大腦。**

StonePotatoes does not aim to simply let AI control NPCs.
StonePotatoes 不僅僅旨在讓 AI 控制 NPC。

**It aims to let AI become life in the world.**
**它旨在讓 AI 成為世界中的生命。**

StonePotatoes does not aim to simply generate maps.
StonePotatoes 不僅僅旨在生成地圖。

**It aims to generate worlds that evolve.**
**它旨在生成會演化的世界。**

StonePotatoes does not aim to simply make games.
StonePotatoes 不僅僅旨在製作遊戲。

**It aims to build worlds that think.**
**它旨在建造會思考的世界。**

---

**StonePotatoes — Build Worlds That Think.**
**StonePotatoes — 建造會思考的世界。**
