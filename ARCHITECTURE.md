# StonePotatoes Architecture Document
# StonePotatoes 架構文件

## Executive Summary
## 執行摘要

StonePotatoes is a next-generation "AI-native, GPU-first, world-simulation, epic game engine" that unifies:

StonePotatoes 是新一代「AI 原生、GPU 優先、世界模擬、史詩級遊戲引擎」，統一了：

- World Engine
  世界引擎
- Game Engine
  遊戲引擎
- AI Engine
  AI 引擎
- Simulation Engine
  模擬引擎
- Civilization Engine
  文明引擎
- Agent Runtime
  代理運行時
- Creation Platform
  創作平台

It is designed to simulate millions of autonomous agents in a living, evolving world with procedural generation, civilization simulation, and emergent narrative.

它設計用於在活著、演化的世界中模擬數百萬個自主代理，具有程序化生成、文明模擬與湧現敘事。

---

## Core Design Philosophy
## 核心設計哲學

### 1. Command Philosophy: Yin-Yang, Five Elements, Reversibility, Strategy Modes
### 1. 指揮哲學：陰陽、五行、可逆性、策略模式

**Yin-Yang Abstraction:**
**陰陽抽象：**
- **Yang (陽):** Public, visible, formal, accountable, institutionalized
  **陽：** 公開、可見、正式、可問責、制度化
- **Yin (陰):** Hidden, invisible, unconventional, surprise, intelligence, strategy
  **陰：** 隱藏、不可見、非傳統、驚喜、情報、策略

**Five Elements (Wu Xing) as World Rhythm:**
**五行作為世界節奏：**
- **Wood (木) - Building:** Establish factions, identity, legitimacy, resources, relationships, goals
  **木 - 建立：** 建立派系、身份、合法性、資源、關係、目標
- **Fire (火) - Expansion:** Acceleration, mobilization, war, propagation, pressure, passion, market expansion
  **火 - 擴張：** 加速、動員、戰爭、傳播、壓力、熱情、市場擴張
- **Earth (土) - Stability:** Governance, resource integration, institutions, cities, organizations, infrastructure
  **土 - 穩定：** 治理、資源整合、制度、城市、組織、基礎設施
- **Metal (金) - Judgment:** War, enforcement, elimination, cleanup, decision, conflict resolution
  **金 - 判斷：** 戰爭、執法、消除、清理、決策、衝突解決
- **Water (水) - Latency:** Intelligence, observation, recovery, momentum accumulation, waiting, hiding, repositioning
  **水 - 潛伏：** 情報、觀察、恢復、動量積累、等待、隱藏、重新定位

**Strategic Cycle:** Building → Expansion → Stability → Judgment → Latency → Rebuilding
**策略循環：** 建立 → 擴張 → 穩定 → 判斷 → 潛伏 → 重建

### 2. AI as Autonomous Agents, Not Scripted NPCs
### 2. AI 作為自主代理，而非腳本 NPC

Each significant NPC possesses:
每個重要的 NPC 擁有：

- Identity, Memory, Personality, Values, Needs, Desires, Fears, Beliefs
  身份、記憶、個性、價值觀、需求、慾望、恐懼、信念
- Relationships, Resources, Intelligence, Knowledge, Misconceptions
  關係、資源、情報、知識、誤解
- Goals (long-term and short-term), Self-awareness
  目標（長期與短期）、自我意識
- Models of other characters and the world
  對其他角色與世界的模型

**Decision Basis:** Agents make decisions based on:
**決策基礎：** 代理基於以下進行決策：

- Vision (what they can see)
  視覺（他們能看到什麼）
- Intelligence (what they know)
  情報（他們知道什麼）
- Experience (what they remember)
  經驗（他們記得什麼）
- Reasoning (what they infer)
  推理（他們推斷什麼）

**No God Knowledge:** NPCs do not know developer-set "truths."
**無上帝知識：** NPC 不知道開發者設定的「真理」。

### 3. True World Model - Living World
### 3. 真實世界模型 - 活著的世界

The world operates continuously:
世界持續運作：

- Cities function, NPCs live, commerce trades, nations diplomacy
  城市運作、NPC 生活、商業貿易、國家外交
- Armies move, wars occur, markets fluctuate, religions spread
  軍隊移動、戰爭發生、市場波動、宗教傳播
- Technology develops, civilizations rise and fall
  科技發展、文明興衰

**Player Role:** The player is just one agent in the world, not the center.
**玩家角色：** 玩家只是世界中的一個代理，不是中心。

### 4. Civilization System
### 4. 文明系統

Complete civilization simulation with:
完整的文明模擬，包含：

- Political, Economic, Military, Technological, Cultural, Religious, Legal systems
  政治、經濟、軍事、科技、文化、宗教、法律系統
- Population, Resources, Territory, Diplomacy, Intelligence, Social Classes
  人口、資源、領土、外交、情報、社會階級
- Organizations, Cities, Infrastructure
  組織、城市、基礎設施

**Civilization Dynamics:** Rise, Split, Merge, Revolution, Reform, Expansion, Decline, Destruction, Revival
**文明動態：** 興起、分裂、合併、革命、改革、擴張、衰退、毀滅、復興

**No Hardcoded Drama:** All outcomes emerge from world state and agent actions.
**無硬編劇情：** 所有結果從世界狀態與代理行動中湧現。

### 5. "Governance" System (治平者)
### 5. 「治理」系統（治平者）

Beyond "War = Victory," incorporate:
超越「戰爭 = 勝利」，納入：

- Governance, Balance, Negotiation, Coexistence, Institutions, Long-term Stability
  治理、平衡、談判、共存、制度、長期穩定

**Victory Strategies:**
**勝利策略：**

- Conquest, Deterrence, Alliance, Trade, Diplomacy, Peace, Federation
  征服、威懾、同盟、貿易、外交、和平、聯邦
- Empire, City-state, Democracy, Monarchy, Religious Governance, AI Governance, Anarchy, Mixed Systems
  帝國、城邦、民主、君主制、宗教治理、AI 治理、無政府、混合系統

**Non-Military Victory:**
**非軍事勝利：**

- "Victory without war"
  「不戰而勝」
- Enemies voluntarily join
  敵人自願加入
- Enemy civilizations convert to allies
  敵對文明轉為盟友
- Economic and cultural domination over military conquest
  經濟與文化支配優於軍事征服
- Deter enemy willingness to fight
  阻遏敵人戰意

**Multi-dimensional Victory Conditions:**
**多維勝利條件：**

- Survival, Prosperity, Peace, Civilization Continuity, Cultural Influence
  生存、繁榮、和平、文明延續、文化影響力
- Economic Hegemony, Technological Hegemony, Military Hegemony, Diplomatic Hegemony
  經濟霸權、科技霸權、軍事霸權、外交霸權
- Ecological Balance, Population Happiness, World Unification, World Autonomy
  生態平衡、人口幸福、世界統一、世界自治
- Transcending traditional win/loss
  超越傳統勝負

### 6. Good/Evil as Strategy Mode, Not Alignment
### 6. 善惡作為策略模式，而非陣營

**Strategy Spectrum:**
**策略光譜：**

- Regular/Unconventional, Public/Hidden, Constructive/Destructive
  常規/非傳統、公開/隱藏、建設/破壞
- Cooperative/Manipulative, Deterrent/Inducement, Direct/Indirect
  合作/操縱、威懾/誘導、直接/間接

**Self-Control System:**
**自我控制系統：**

- When unconventional means can be used
  何時可使用非傳統手段
- When to stop using them
  何時停止使用
- Costs of unconventional means
  非傳統手段的成本
- Long-term corruption risks
  長期腐敗風險
- Trust loss, organizational degradation, social backlash
  信任流失、組織退化、社會反彈

**Corruption/Degradation System:** Strategies produce long-term consequences.
**腐敗/退化系統：** 策略產生長期後果。

### 7. Six AI Strategic Dimensions
### 7. 六個 AI 策略維度

1. **Mind:** Will, Belief, Loyalty, Fear, Morale
   **心智：** 意志、信念、忠誠、恐懼、士氣
2. **Morale:** Spirit, Will to Fight, Organizational Stability, Group Confidence
   **士氣：** 精神、戰意、組織穩定、群體信心
3. **Information:** Intelligence, Misinformation, False Information, Vision, Cognition
   **情報：** 情報、誤報、虛假資訊、視覺、認知
4. **Infrastructure:** Production, Supply, Cities, Economy, Technology
   **基礎設施：** 生產、供應、城市、經濟、科技
5. **Momentum:** Strategic Initiative, Timing, Action Speed, Battlefield Advantage
   **動量：** 戰略主動權、時機、行動速度、戰場優勢
6. **Unknown:** Unknown, Black Swan, Uncertainty, Hidden Intelligence, Hidden Forces
   **未知：** 未知、黑天鵝、不確定性、隱藏情報、隱藏力量

AI must make decisions regarding the Unknown.
AI 必須對未知做出決策。

---

## Overall Architecture
## 整體架構

### High-Level System Graph
### 高層系統圖

```
┌─────────────────────────────────────────────────────────────────┐
│                         StonePotatoes                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │   Rendering  │  │    Physics   │  │   AI Engine  │          │
│  │    Engine    │  │    Engine    │  │              │          │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘          │
│         │                 │                 │                   │
│  ┌──────▼─────────────────▼─────────────────▼───────┐          │
│  │              GPU Compute Layer                  │          │
│  │         (Rendering, Physics, AI, Simulation)     │          │
│  └────────────────────┬────────────────────────────┘          │
│                       │                                        │
│  ┌────────────────────▼────────────────────────────┐          │
│  │                 ECS Core                         │          │
│  │  (Entity-Component-System + World State)         │          │
│  └────────────────────┬────────────────────────────┘          │
│                       │                                        │
│  ┌────────────────────▼────────────────────────────┐          │
│  │            World Simulation Layer               │          │
│  │  (Time, History, Ecology, Geography, Climate)   │          │
│  └────────────────────┬────────────────────────────┘          │
│                       │                                        │
│  ┌────────────────────▼────────────────────────────┐          │
│  │         Civilization Simulation Layer           │          │
│  │  (Politics, Economy, Military, Tech, Culture)   │          │
│  └────────────────────┬────────────────────────────┘          │
│                       │                                        │
│  ┌────────────────────▼────────────────────────────┐          │
│  │              Agent Runtime Layer                 │          │
│  │  (Layer 0-5 AI Hierarchy + Memory + Relations)   │          │
│  └────────────────────┬────────────────────────────┘          │
│                       │                                        │
│  ┌────────────────────▼────────────────────────────┐          │
│  │                Game Engine API                   │          │
│  │  (Gameplay, Input, UI, Audio, Networking)        │          │
│  └────────────────────┬────────────────────────────┘          │
│                       │                                        │
│  ┌────────────────────▼────────────────────────────┐          │
│  │              Platform / OS Layer                 │          │
│  │  (Window, IO, Threading, File System)           │          │
│  └─────────────────────────────────────────────────┘          │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### GPU First Architecture
### GPU 優先架構

**CPU Responsibilities:**
**CPU 職責：**

- OS abstraction
  作業系統抽象
- I/O operations
  I/O 操作
- Gameplay API
  遊戲玩法 API
- High-complexity control flow
  高複雜度控制流
- Networking
  網路
- Toolchain
  工具鏈

**GPU Responsibilities:**
**GPU 職責：**

- Rendering
  渲染
- Physics
  物理
- Particle systems
  粒子系統
- Crowd simulation
  群眾模擬
- World simulation
  世界模擬
- AI inference
  AI 推論
- Pathfinding
  路徑規劃
- Visibility computation
  可見性計算
- Procedural generation
  程序化生成
- Large-scale agent simulation
  大規模代理模擬

**AI Accelerator (NPU/TPU):**
**AI 加速器（NPU/TPU）：**

- Tensor operations
  張量運算
- Neural network inference
  神經網路推論
- Embeddings
  嵌入
- Vector search
  向量搜尋
- Agent evaluation
  代理評估

---

## Module Graph
## 模組圖

### Core Modules
### 核心模組

```
Core
├── Platform (Window, IO, Threading, File System)
├── Memory (Memory Pool, Allocator, Cache)
├── Time (Multi-scale time system)
├── Config (Configuration management)
├── Logging (Debug logging, profiling)
└── Plugin (Plugin system, hot-reload)

ECS Core
├── Entity (Entity ID, Archetype, Generation)
├── Component (Component registration, storage)
├── System (System scheduler, parallel execution)
├── Query (Component queries, filters)
├── Events (Event bus, subscriptions)
└── World (World state, entity management)

Rendering Engine
├── RHI (Render Hardware Interface)
│   ├── DirectX 12
│   ├── Vulkan
│   └── Metal
├── Graphics (PBR, HDR, GI, Ray Tracing, Path Tracing)
├── Virtual Geometry (Nanite-like streaming)
├── Virtual Texturing (Streaming system)
├── Post-Processing (Bloom, DOF, Motion Blur, etc.)
├── UI (Immediate mode UI, editor UI)
└── Debug Visualization (Debug draw, profiling overlay)

Physics Engine
├── Rigid Body (Dynamics, collision)
├── Soft Body (Deformation, cloth)
├── Fluid (Water, gases)
├── Destruction (Building destruction, terrain)
├── Vehicle (Vehicle simulation)
├── Character (Character controller)
└── GPU Physics (GPU-accelerated simulation)

AI Engine
├── Agent Runtime (Agent lifecycle, execution)
├── AI Hierarchy (Layer 0-5 AI systems)
├── Memory System (Short-term, Episodic, Semantic, Social, Trauma)
├── Relationship Graph (Dynamic relationship network)
├── Decision System (Utility-based, planning, review)
├── Perception (Vision, hearing, intelligence)
├── Behavior (Behavior trees, state machines, neural networks)
└── Learning (Reinforcement learning, evolution)

World Simulation
├── Time System (Frame, Minute, Hour, Day, Month, Year, Era)
├── Geography (Terrain, climate, resources)
├── Ecology (Plants, animals, food chains)
├── Weather (Weather patterns, climate change)
├── History (World chronicle, event logging)
├── Procedural Generation (World, cities, content)
└── Ecosystem (Resource cycles, environmental impact)

Civilization Simulation
├── Political System (Governance, laws, factions)
├── Economic System (Trade, markets, production)
├── Military System (Armies, wars, logistics)
├── Technology System (Research, innovation)
├── Cultural System (Religion, art, values)
├── Population System (Demographics, migration)
├── Diplomacy System (Relations, treaties, alliances)
└── Intelligence System (Espionage, counter-intelligence)

Agent Systems
├── NPC Generator (Procedural NPC creation)
├── AI Director (World event generation)
├── Narrative Engine (Emergent story generation)
├── Quest System (Dynamic quest generation)
├── Dialogue System (AI-generated dialogue)
└── Reputation System (World reputation, faction relations)

Networking
├── Server (Dedicated server, authoritative simulation)
├── Client (Rendering client, prediction)
├── Replication (State synchronization)
├── Entity Replication (Networked entities)
├── RPC (Remote procedure calls)
├── Cloud Simulation (Distributed world simulation)
└── World Sharding (Region-based simulation)

Data & Assets
├── Asset Pipeline (Import, processing, cooking)
├── Resource Management (Loading, streaming, caching)
├── Serialization (Save/Load, state persistence)
├── Database (Persistent storage, analytics)
└── Version Control (Asset versioning, diff storage)

Editor & Tools
├── World Editor (World building, terrain editing)
├── Civilization Editor (Civilization configuration)
├── NPC Designer (NPC creation, behavior design)
├── AI Behavior Designer (AI debugging, visualization)
├── Agent Debugger (Agent inspection, thought visualization)
├── World Inspector (World state inspection)
├── Timeline (History visualization, time control)
└── Profiler (Performance profiling, optimization)

Developer API
├── C++ API (Native, high-performance)
├── Rust API (Safe, systems programming)
├── Python API (Scripting, AI integration)
├── TypeScript API (Web, tooling)
├── Lua API (Gameplay scripting)
├── REST API (Web services)
├── WebSocket API (Real-time communication)
└── Plugin API (Third-party extensions)

Mod System
├── Mod Loader (Mod discovery, loading)
├── Mod API (Mod interface, sandboxing)
├── Content Modding (Asset modification)
├── Gameplay Modding (Rule modification)
├── AI Modding (AI behavior modification)
└── World Modding (World generation modification)
```

---

## ECS Architecture
## ECS 架構

### Design Principles
### 設計原則

- **Data-Oriented Design:** Optimize for cache locality and SIMD
  **資料導向設計：** 優化快取區域性與 SIMD
- **Entity-Component-System:** Separate data from logic
  **實體-組件-系統：** 資料與邏輯分離
- **Archetype-based Storage:** Group entities by component composition
  **基於原型的儲存：** 按組件組成分組實體
- **Parallel Execution:** Systems run in parallel where possible
  **平行執行：** 系統盡可能平行執行
- **Deterministic Simulation:** Same inputs produce same outputs
  **確定性模擬：** 相同輸入產生相同輸出

### ECS Core Structure
### ECS 核心結構

```
Entity
├── ID (64-bit: Index [32] + Generation [32])
├── Archetype (Component composition)
└── Components (Data stored in archetype chunks)

Component
├── Type ID (Unique component type identifier)
├── Size (Component size in bytes)
├── Alignment (Memory alignment)
└── Registration (Component metadata)

Archetype
├── Component Types (Set of component types)
├── Chunks (Memory blocks for entity data)
├── Entity Table (Entity → Chunk + Index mapping)
└── Chunk Allocator (Memory management)

System
├── Query (Component query specification)
├── Execution (System execution logic)
├── Dependencies (System execution order)
├── Parallelism (Thread-safe execution)
└── Resources (System resources, world access)

Query
├── Read Components (Components to read)
├── Write Components (Components to write)
├── Filter (Optional component filters)
└── Iterator (Efficient iteration over matching entities)

World
├── Entities (Entity manager)
├── Components (Component manager)
├── Archetypes (Archetype manager)
├── Systems (System scheduler)
├── Resources (Resource storage)
├── Events (Event bus)
└── Time (World time state)
```

### Example Components
### 範例組件

**Transform Components:**
**變換組件：**

- `Transform` (Position, Rotation, Scale)
  `Transform`（位置、旋轉、縮放）
- `Velocity` (Linear velocity, Angular velocity)
  `Velocity`（線速度、角速度）
- `Acceleration` (Linear acceleration, Angular acceleration)
  `Acceleration`（線加速度、角加速度）

**Physics Components:**
**物理組件：**

- `RigidBody` (Mass, Inertia, Collision shape)
  `RigidBody`（質量、慣性、碰撞形狀）
- `Collider` (Collision geometry, material)
  `Collider`（碰撞幾何、材質）
- `CharacterController` (Character movement)
  `CharacterController`（角色移動）

**AI Components:**
**AI 組件：**

- `AIState` (Current AI state, mode)
  `AIState`（當前 AI 狀態、模式）
- `Memory` (Short-term, episodic, semantic memory)
  `Memory`（短期、情節、語意記憶）
- `Needs` (Hunger, thirst, safety, social)
  `Needs`（飢餓、口渴、安全、社交）
- `Goals` (Current goals, priorities)
  `Goals`（當前目標、優先級）
- `Personality` (Traits, values, preferences)
  `Personality`（特質、價值觀、偏好）
- `Relationships` (Relationship graph node)
  `Relationships`（關係圖節點）
- `Knowledge` (World knowledge, facts)
  `Knowledge`（世界知識、事實）
- `Beliefs` (Beliefs, misconceptions)
  `Beliefs`（信念、誤解）

**Civilization Components:**
**文明組件：**

- `Faction` (Faction membership, allegiance)
  `Faction`（派系成員身份、效忠）
- `Reputation` (Reputation with various groups)
  `Reputation`（與各群體的聲譽）
- `Culture` (Cultural background, values)
  `Culture`（文化背景、價值觀）
- `SocialClass` (Social class, status)
  `SocialClass`（社會階級、地位）
- `Occupation` (Job, role, income)
  `Occupation`（職業、角色、收入）

**Resource Components:**
**資源組件：**

- `Inventory` (Items, resources, money)
  `Inventory`（物品、資源、金錢）
- `Resource` (Resource type, amount)
  `Resource`（資源類型、數量）
- `Production` (Production capabilities)
  `Production`（生產能力）

**Gameplay Components:**
**遊戲玩法組件：**

- `Health` (Health, damage, regeneration)
  `Health`（生命值、傷害、再生）
- `Stats` (Attributes, skills, abilities)
  `Stats`（屬性、技能、能力）
- `Action` (Current action, progress)
  `Action`（當前行動、進度）
- `Interaction` (Interaction capabilities)
  `Interaction`（互動能力）

### Example Systems
### 範例系統

**MovementSystem:**
**移動系統：**

- Update position based on velocity
  根據速度更新位置
- Handle collision with terrain
  處理地形碰撞
- Update velocity based on acceleration
  根據加速度更新速度

**CombatSystem:**
**戰鬥系統：**

- Calculate damage between combatants
  計算戰鬥者之間的傷害
- Apply damage to health
  對生命值施加傷害
- Handle death and destruction
  處理死亡與毀滅

**AISystem:**
**AI 系統：**

- Update AI state based on perception
  根據感知更新 AI 狀態
- Execute AI decisions
  執行 AI 決策
- Update memory and knowledge
  更新記憶與知識

**EconomySystem:**
**經濟系統：**

- Process trades and transactions
  處理貿易與交易
- Update market prices
  更新市場價格
- Handle production and consumption
  處理生產與消費

**DiplomacySystem:**
**外交系統：**

- Process diplomatic actions
  處理外交行動
- Update relationships between factions
  更新派系間的關係
- Handle treaties and alliances
  處理條約與同盟

**CivilizationSystem:**
**文明系統：**

- Update civilization state
  更新文明狀態
- Process political decisions
  處理政治決策
- Handle population dynamics
  處理人口動態

**MemorySystem:**
**記憶系統：**

- Update short-term memory
  更新短期記憶
- Consolidate episodic memory
  整合情節記憶
- Decay old memories
  衰減舊記憶

**RelationshipSystem:**
**關係系統：**

- Update relationship values
  更新關係值
- Process social interactions
  處理社交互動
- Handle relationship events
  處理關係事件

**WorldSimulationSystem:**
**世界模擬系統：**

- Update world time
  更新世界時間
- Process ecological changes
  處理生態變化
- Handle weather and climate
  處理天氣與氣候

---

## Rendering Architecture
## 渲染架構

### Design Goals
### 設計目標

- **Next-Generation AAA Rendering:** PBR, HDR, GI, Ray Tracing
  **下一代 AAA 渲染：** PBR、HDR、GI、光線追蹤
- **GPU-First:** Heavy use of GPU compute and tessellation
  **GPU 優先：** 大量使用 GPU 計算與細分
- **Virtualized Geometry:** Nanite-like geometry streaming
  **虛擬化幾何：** Nanite 式幾何串流
- **Virtualized Texturing:** Sparse texture streaming
  **虛擬化紋理：** 稀疏紋理串流
- **Scalable:** From indie to AAA quality levels
  **可擴展：** 從獨立到 AAA 品質等級

### Rendering Pipeline
### 渲染管線

```
Frame Start
├── Culling (Frustum, occlusion, distance)
├── LOD Selection (Geometry, material, shadow)
├── Shadow Pass (Shadow map generation)
├── Depth Pre-pass (Z-prepass)
├── G-Buffer Pass (Deferred rendering)
├── Lighting Pass (Deferred lighting)
├── Forward Pass (Transparency, special materials)
├── Post-Processing (Bloom, DOF, motion blur, etc.)
├── UI Pass (UI rendering)
└── Present (Swap chain)
```

### Rendering Features
### 渲染功能

**Core Rendering:**
**核心渲染：**

- PBR (Physically Based Rendering)
  PBR（基於物理的渲染）
- HDR (High Dynamic Range)
  HDR（高動態範圍）
- Global Illumination (Screen-space, voxel-based)
  全域光照（螢幕空間、體素基礎）
- Ray Tracing (Reflections, shadows, global illumination)
  光線追蹤（反射、陰影、全域光照）
- Path Tracing (Progressive path tracing for offline quality)
  路徑追蹤（用於離線品質的漸進式路徑追蹤）

**Virtual Geometry:**
**虛擬幾何：**

- Nanite-like virtual geometry streaming
  Nanite 式虛擬幾何串流
- Mesh shaders for GPU-driven culling
  用於 GPU 驅動剔除的網格著色器
- Progressive mesh refinement
  漸進式網格精細化
- Automatic LOD generation
  自動 LOD 生成

**Virtual Texturing:**
**虛擬紋理：**

- Sparse texture streaming
  稀疏紋理串流
- Tile-based texture caching
  基於磚塊的紋理快取
- Procedural texture generation
  程序化紋理生成
- Texture compression
  紋理壓縮

**Post-Processing:**
**後處理：**

- Bloom
  輝光
- Depth of Field
  景深
- Motion Blur
  動態模糊
- Film Grain
  膠片顆粒
- Chromatic Aberration
  色差
- Color Grading
  色彩分級
- Tone Mapping
  色調映射
- Anti-Aliasing (TAA, FXAA, MSAA)
  反鋸齒（TAA、FXAA、MSAA）

**Special Effects:**
**特效：**

- Volumetric Fog
  體積霧
- Volumetric Clouds
  體積雲
- Volumetric Lighting
  體積光照
- Ocean/Water simulation
  海洋/水模擬
- Terrain rendering
  地形渲染
- Destruction rendering
  毀滅渲染
- Hair rendering
  頭髮渲染
- Skin rendering
  皮膚渲染
- Particle systems
  粒子系統

**Animation:**
**動畫：**

- Skeletal animation
  骨骼動畫
- Blend shapes
  混合形狀
- Procedural animation
  程序化動畫
- Inverse kinematics
  逆運動學
- Animation blending
  動畫混合
- Massive crowd animation
  大規模群眾動畫

### RHI (Render Hardware Interface)
### RHI（渲染硬體介面）

**Supported APIs:**
**支援的 API：**

- DirectX 12 (Windows)
  DirectX 12（Windows）
- Vulkan (Cross-platform)
  Vulkan（跨平台）
- Metal (macOS, iOS)
  Metal（macOS、iOS）

**RHI Features:**
**RHI 功能：**

- Abstracted GPU commands
  抽象化 GPU 指令
- Resource management
  資源管理
- Shader compilation and management
  著色器編譯與管理
- Pipeline state objects
  管線狀態物件
- Descriptor heaps
  描述符堆
- Command lists and bundles
  指令清單與捆綁
- Synchronization and fences
  同步與柵欄

---

## Physics Architecture
## 物理架構

### Design Goals
### 設計目標

- **GPU-Accelerated:** Large-scale simulation on GPU
  **GPU 加速：** GPU 上大規模模擬
- **Deterministic:** Reproducible simulation results
  **確定性：** 可重現的模擬結果
- **Scalable:** From character to building destruction
  **可擴展：** 從角色到建築毀滅
- **Integration:** Deep integration with ECS and world simulation
  **整合：** 與 ECS 和世界模擬深度整合

### Physics Systems
### 物理系統

**Rigid Body Physics:**
**剛體物理：**

- Rigid body dynamics
  剛體動力學
- Collision detection and response
  碰撞檢測與回應
- Constraints (joints, motors)
  約束（關節、馬達）
- Collision filtering and layers
  碰撞過濾與層級

**Soft Body Physics:**
**軟體物理：**

- Cloth simulation
  布料模擬
- Deformable objects
  可變形物件
- Soft body dynamics
  軟體動力學

**Fluid Physics:**
**流體物理：**

- Water simulation
  水模擬
- Gas simulation
  氣體模擬
- Fluid dynamics
  流體動力學

**Destruction:**
**毀滅：**

- Building destruction
  建築毀滅
- Terrain destruction
  地形毀滅
- Fracture simulation
  斷裂模擬
- Debris generation
  碎片生成

**Vehicle Physics:**
**載具物理：**

- Vehicle dynamics
  載具動力學
- Wheel simulation
  輪子模擬
- Suspension
  懸吊
- Aerodynamics
  空氣動力學

**Character Physics:**
**角色物理：**

- Character controller
  角色控制器
- Animation-driven physics
  動畫驅動物理
- Ragdoll physics
  布娃娃物理

**Rope Physics:**
**繩索物理：**

- Rope simulation
  繩索模擬
- Chain simulation
  鏈條模擬
- Cable dynamics
  纜線動力學

**Particle Physics:**
**粒子物理：**

- Particle simulation
  粒子模擬
- Collision with particles
  與粒子碰撞
- Particle effects
  粒子特效

### GPU Physics
### GPU 物理

**GPU-Accelerated Systems:**
**GPU 加速系統：**

- Rigid body simulation (GPU collision, GPU dynamics)
  剛體模擬（GPU 碰撞、GPU 動力學）
- Particle simulation (GPU particles)
  粒子模擬（GPU 粒子）
- Cloth simulation (GPU cloth)
  布料模擬（GPU 布料）
- Fluid simulation (GPU fluid)
  流體模擬（GPU 流體）
- Destruction (GPU fracture)
  毀滅（GPU 斷裂）

**GPU Compute Features:**
**GPU 計算功能：**

- Broad phase collision detection (GPU spatial partitioning)
  廣相碰撞檢測（GPU 空間分割）
- Narrow phase collision detection (GPU collision testing)
  窄相碰撞檢測（GPU 碰撞測試）
- Constraint solving (GPU constraint solver)
  約束求解（GPU 約束求解器）
- Integration (GPU position/velocity update)
  積分（GPU 位置/速度更新）

---

## AI Architecture
## AI 架構

### Design Goals
### 設計目標

- **Autonomous Agents:** Agents make independent decisions
  **自主代理：** 代理獨立做出決策
- **Multi-Layer AI:** Reflex to civilization-level AI
  **多層 AI：** 從反射到文明級 AI
- **Memory-Driven:** AI based on memory, experience, knowledge
  **記憶驅動：** AI 基於記憶、經驗、知識
- **Emergent Behavior:** Complex behavior from simple rules
  **湧現行為：** 從簡單規則產生複雜行為
- **Scalable:** Support millions of agents
  **可擴展：** 支援數百萬代理

### AI Agent Hierarchy
### AI 代理階層

**Layer 0 — Reflex AI:**
**第 0 層 — 反射 AI：**

- Immediate reactions
  立即反應
- Actions
  行動
- Collision avoidance
  碰撞避免
- Instant combat
  即時戰鬥
- **Execution:** Every frame
  **執行：** 每幀
- **Scope:** Individual entity
  **範圍：** 個體實體

**Layer 1 — Tactical AI:**
**第 1 層 — 戰術 AI：**

- Squad coordination
  小隊協調
- Tactical decisions
  戰術決策
- Cover and positioning
  掩護與定位
- Focus fire
  集中火力
- **Execution:** Combat tick (100ms)
  **執行：** 戰鬥 tick（100ms）
- **Scope:** Small group (5-20 entities)
  **範圍：** 小群體（5-20 個實體）

**Layer 2 — Operational AI:**
**第 2 層 — 作戰 AI：**

- Army management
  軍隊管理
- City management
  城市管理
- Resource allocation
  資源分配
- Mission planning
  任務規劃
- **Execution:** Game minute
  **執行：** 遊戲分鐘
- **Scope:** Medium group (20-200 entities)
  **範圍：** 中群體（20-200 個實體）

**Layer 3 — Strategic AI:**
**第 3 層 — 策略 AI：**

- National strategy
  國家策略
- War planning
  戰爭規劃
- Diplomacy
  外交
- Economic policy
  經濟政策
- **Execution:** Game hour
  **執行：** 遊戲小時
- **Scope:** Large group (200-2000 entities)
  **範圍：** 大群體（200-2000 個實體）

**Layer 4 — Civilization AI:**
**第 4 層 — 文明 AI：**

- Civilization development
  文明發展
- Cultural direction
  文化方向
- Long-term evolution
  長期演化
- Historical direction
  歷史方向
- **Execution:** Game day
  **執行：** 遊戲日
- **Scope:** Civilization
  **範圍：** 文明

**Layer 5 — World AI:**
**第 5 層 — 世界 AI：**

- Global events
  全球事件
- Ecological balance
  生態平衡
- Weather and climate
  天氣與氣候
- Large-scale crises
  大規模危機
- Civilization balance
  文明平衡
- **Execution:** Game month
  **執行：** 遊戲月
- **Scope:** Entire world
  **範圍：** 整個世界

### Agent Components
### 代理組件

**Identity:**
**身份：**

- Name, age, gender
  姓名、年齡、性別
- Background, history
  背景、歷史
- Personality traits
  個性特質
- Values, beliefs
  價值觀、信念

**Memory:**
**記憶：**

- Short-term memory (recent events)
  短期記憶（最近事件）
- Episodic memory (important experiences)
  情節記憶（重要經驗）
- Semantic memory (world knowledge)
  語意記憶（世界知識）
- Social memory (impressions of others)
  社交記憶（對他人的印象）
- Trauma/experience (impactful events)
  創傷/經驗（影響深遠的事件）

**Needs:**
**需求：**

- Physiological (hunger, thirst, sleep)
  生理（飢餓、口渴、睡眠）
- Safety (security, shelter)
  安全（安全、庇護）
- Social (belonging, relationships)
  社交（歸屬、關係）
- Esteem (respect, status)
  尊重（尊重、地位）
- Self-actualization (goals, purpose)
  自我實現（目標、目的）

**Goals:**
**目標：**

- Long-term goals (months/years)
  長期目標（月/年）
- Short-term goals (days/weeks)
  短期目標（日/週）
- Immediate goals (hours/minutes)
  立即目標（時/分）
- Goal priorities
  目標優先級

**Knowledge:**
**知識：**

- World facts
  世界事實
- Geographic knowledge
  地理知識
- Social knowledge
  社會知識
- Cultural knowledge
  文化知識
- Technical knowledge
  技術知識

**Beliefs:**
**信念：**

- Personal beliefs
  個人信念
- Misconceptions
  誤解
- Biases
  偏見
- Assumptions
  假設

**Relationships:**
**關係：**

- Relationship graph node
  關係圖節點
- Relationship values (trust, fear, respect, love, hatred)
  關係值（信任、恐懼、尊重、愛、仇恨）
- Relationship history
  關係歷史

**Resources:**
**資源：**

- Inventory
  庫存
- Money
  金錢
- Assets
  資產
- Social capital
  社會資本

**Perception:**
**感知：**

- Vision (what they can see)
  視覺（他們能看到什麼）
- Hearing (what they can hear)
  聽覺（他們能聽到什麼）
- Intelligence (what they know)
  情報（他們知道什麼）
- Misconceptions (what they believe incorrectly)
  誤解（他們錯誤相信什麼）

### Decision System
### 決策系統

**Utility-Based Decision Making:**
**基於效用的決策制定：**

- Evaluate options based on utility scores
  根據效用分數評估選項
- Consider multiple factors (needs, goals, risks, costs)
  考慮多個因素（需求、目標、風險、成本）
- Select best option
  選擇最佳選項

**Planning:**
**規劃：**

- Goal decomposition
  目標分解
- Action planning
  行動規劃
- Path planning
  路徑規劃
- Resource planning
  資源規劃

**Decision Review:**
**決策審查：**

- Post-decision evaluation
  決策後評估
- Learning from outcomes
  從結果中學習
- Updating beliefs and knowledge
  更新信念與知識
- Adjusting strategy
  調整策略

**Reversibility Score:**
**可逆性分數：**

- Evaluate reversibility of decisions
  評估決策的可逆性
- Reversible (low cost)
  可逆（低成本）
- Partially reversible (medium cost)
  部分可逆（中成本）
- Irreversible (high cost)
  不可逆（高成本）

### Memory System
### 記憶系統

**Short-Term Memory:**
**短期記憶：**

- Recent events (last few minutes/hours)
  最近事件（過去幾分鐘/小時）
- Fast access
  快速存取
- Rapid decay
  快速衰減

**Episodic Memory:**
**情節記憶：**

- Important experiences
  重要經驗
- Contextual (time, place, people)
  有情境（時間、地點、人物）
- Slow decay
  慢速衰減
- Long-term retention
  長期保留

**Semantic Memory:**
**語意記憶：**

- World knowledge (facts, concepts)
  世界知識（事實、概念）
- Abstract knowledge
  抽象知識
- Very slow decay
  非常慢的衰減
- Can be updated
  可更新

**Social Memory:**
**社交記憶：**

- Impressions of others
  對他人的印象
- Relationship history
  關係歷史
- Social interactions
  社交互動
- Influences decisions about others
  影響對他人的決策

**Trauma/Experience:**
**創傷/經驗：**

- Impactful events
  影響深遠的事件
- Long-lasting influence
  長期影響
- Can affect personality and beliefs
  可影響個性與信念
- Example: War trauma affecting political stance decades later
  範例：戰爭創傷在數十年後影響政治立場

### Relationship Graph
### 關係圖

**Nodes:**
**節點：**

- NPCs
  NPC
- Families
  家庭
- Cities
  城市
- Organizations
  組織
- Nations
  國家
- Religions
  宗教
- Companies
  公司
- Armies
  軍隊

**Edges:**
**邊：**

- Friend, Enemy
  朋友、敵人
- Parent, Child
  父母、子女
- Ally, Rival
  盟友、對手
- Debt
  債務
- Trust, Fear
  信任、恐懼
- Love, Hatred
  愛、仇恨
- Respect
  尊重
- Dependency
  依賴

**Dynamic Evolution:**
**動態演化：**

- Relationships change over time
  關係隨時間變化
- Influenced by actions, events, reputation
  受行動、事件、聲譽影響
- Can decay or strengthen
  可衰減或增強

### Perception System
### 感知系統

**Vision:**
**視覺：**

- Line of sight
  視線
- Distance-based visibility
  基於距離的可見性
- Occlusion
  遮擋
- Lighting conditions
  光照條件

**Hearing:**
**聽覺：**

- Sound propagation
  聲音傳播
- Distance-based hearing
  基於距離的聽覺
- Obstruction
  障礙
- Environmental noise
  環境噪音

**Intelligence:**
**情報：**

- Information from other agents
  來自其他代理的資訊
- Rumors
  謠言
- Reports
  報告
- Reconnaissance
  偵察

**Misconceptions:**
**誤解：**

- False information
  虛假資訊
- Misinterpretation
  誤解
- Outdated information
  過時資訊
- Biased perception
  偏見感知

### Learning System
### 學習系統

**Reinforcement Learning:**
**強化學習：**

- Learn from rewards and punishments
  從獎懲中學習
- Update behavior based on outcomes
  根據結果更新行為
- Long-term optimization
  長期優化

**Evolution:**
**演化：**

- Population-level learning
  群體層級學習
- Cultural evolution
  文化演化
- Knowledge transmission
  知識傳遞

**Imitation:**
**模仿：**

- Learn from observing others
  從觀察他人中學習
- Social learning
  社會學習
- Cultural transmission
  文化傳遞

---

## Agent Architecture
## 代理架構

### Agent Lifecycle
### 代理生命週期

```
Spawn
├── Generate identity (procedural generation)
├── Initialize memory (blank or inherited)
├── Set initial needs
├── Set initial goals
├── Set initial relationships
└── Place in world

Update (Per Tick)
├── Perception (gather information)
├── Memory update (store new information)
├── Need evaluation (assess current needs)
├── Goal evaluation (assess current goals)
├── Decision making (choose action)
├── Action execution (perform action)
├── Memory consolidation (update long-term memory)
└── Relationship update (update relationships)

Death
├── Stop updates
├── Memory cleanup
├── Relationship cleanup
├── Resource cleanup
└── Remove from world
```

### Agent LOD (Level of Detail)
### 代理 LOD（細節層級）

**LOD 0 - Full Simulation (Near Player):**
**LOD 0 - 完整模擬（玩家附近）：**

- Full AI hierarchy
  完整 AI 階層
- Full memory system
  完整記憶系統
- Full perception
  完整感知
- High-fidelity behavior
  高保真行為
- **Execution:** Every frame
  **執行：** 每幀

**LOD 1 - Reduced Simulation (Medium Distance):**
**LOD 1 - 簡化模擬（中等距離）：**

- Reduced AI hierarchy (Layers 0-2)
  簡化 AI 階層（第 0-2 層）
- Simplified memory
  簡化記憶
- Simplified perception
  簡化感知
- Medium-fidelity behavior
  中保真行為
- **Execution:** Every few frames
  **執行：** 每幾幀

**LOD 2 - Minimal Simulation (Far Distance):**
**LOD 2 - 最小模擬（遠距離）：**

- Minimal AI (Layers 0-1)
  最小 AI（第 0-1 層）
- Minimal memory
  最小記憶
- No perception
  無感知
- Low-fidelity behavior
  低保真行為
- **Execution:** Every second
  **執行：** 每秒

**LOD 3 - Abstract Simulation (Very Far):**
**LOD 3 - 抽象模擬（非常遠）：**

- Statistical simulation
  統計模擬
- No individual behavior
  無個體行為
- Group-level behavior
  群體層級行為
- **Execution:** Every minute
  **執行：** 每分鐘

**LOD 4 - Sleep (Inactive):**
**LOD 4 - 休眠（非活動）：**

- No simulation
  無模擬
- Stored in database
  儲存在資料庫
- Wakes up when needed
  需要時喚醒
- **Execution:** None
  **執行：** 無

### Agent Sleep/Wake
### 代理休眠/喚醒

**Sleep Conditions:**
**休眠條件：**

- Far from player
  遠離玩家
- In inactive area
  在非活動區域
- Low importance
- 低重要性
- Long inactivity
  長期不活動

**Wake Conditions:**
**喚醒條件：**

- Player approaches
  玩家接近
- Important event occurs
  重要事件發生
- High importance
  高重要性
- Time-triggered wake
  時間觸發喚醒

---

## World Simulation
## 世界模擬

### Multi-Scale Time System
### 多尺度時間系統

**Frame:**
**幀：**

- Millisecond-scale
  毫秒級
- Combat tick
  戰鬥 tick
- Real-time interactions
  即時互動
- **Duration:** ~16ms (60 FPS)
  **持續時間：** ~16ms（60 FPS）

**Game Minute:**
**遊戲分鐘：**

- NPC activities
  NPC 活動
- Short-term decisions
  短期決策
- Local events
  本地事件
- **Duration:** 1 real second = 1 game minute
  **持續時間：** 1 真實秒 = 1 遊戲分鐘

**Game Hour:**
**遊戲小時：**

- Economic updates
  經濟更新
- City operations
  城市運作
- Military movements
  軍事移動
- **Duration:** 1 real minute = 1 game hour
  **持續時間：** 1 真實分 = 1 遊戲小時

**Game Day:**
**遊戲日：**

- Population dynamics
  人口動態
- Social interactions
  社交互動
- Daily routines
  日常例行
- **Duration:** 1 real hour = 1 game day
  **持續時間：** 1 真實時 = 1 遊戲日

**Game Month:**
**遊戲月：**

- Political decisions
  政治決策
- Market cycles
  市場週期
- Technology progress
  科技進展
- **Duration:** 1 real day = 1 game month
  **持續時間：** 1 真實日 = 1 遊戲月

**Game Year:**
**遊戲年：**

- Civilization evolution
  文明演化
- Historical events
  歷史事件
- Long-term trends
  長期趨勢
- **Duration:** 1 real week = 1 game year
  **持續時間：** 1 真實週 = 1 遊戲年

**Era:**
**紀元：**

- Civilization evolution
  文明演化
- Epochal changes
- 紀元變化
- **Duration:** Variable
  **持續時間：** 可變

**Time Control:**
**時間控制：**

- Pause / Resume
  暫停 / 繼續
- Fast Forward (2x, 4x, 8x, 16x)
  快進（2x、4x、8x、16x）
- Rewind (limited, requires state snapshots)
  倒轉（有限，需要狀態快照）
- Branch (alternate timelines)
  分支（替代時間線）

### World History System
### 世界歷史系統

**Event Logging:**
**事件記錄：**

- Every significant event is logged
  每個重要事件都被記錄
- Events have timestamp, location, participants
  事件有時間戳、位置、參與者
- Events are categorized (political, military, economic, cultural)
  事件被分類（政治、軍事、經濟、文化）

**World Chronicle:**
**世界編年史：**

- Automatically generated history
  自動生成的歷史
- Player can view past events
  玩家可以查看過去事件
- Timeline visualization
  時間軸視覺化
- Historical analysis
  歷史分析

**Event Types:**
**事件類型：**

- Civilization founded/destroyed
  文明建立/毀滅
- Hero born/died
  英雄出生/死亡
- City captured/lost
  城市被佔領/失去
- Technology invented
  科技發明
- Religion founded
  宗教建立
- Dynasty overthrown
- 朝代推翻
- War started/ended
  戰爭開始/結束
- Trade established/broken
  貿易建立/斷絕

### Procedural World Generation
### 程序化世界生成

**Geography:**
**地理：**

- Terrain generation (heightmaps, biomes)
  地形生成（高度圖、生物群系）
- Mountain ranges
  山脈
- Rivers and lakes
  河流與湖泊
- Oceans and seas
  海洋與海
- Climate zones
  氣候帶

**Resources:**
**資源：**

- Mineral deposits
  礦床
- Forests
  森林
- Water sources
  水源
- Fertile land
  肥沃土地
- Rare resources
  稀有資源

**Cities:**
**城市：**

- City placement (geography-based)
  城市位置（基於地理）
- Road networks
  道路網路
- Village locations
  村庄位置
- Trade routes
  貿易路線

**Causality Chain:**
**因果鏈：**

- Geography → Economy → Civilization → Politics → War
  地理 → 經濟 → 文明 → 政治 → 戰爭

### Ecosystem
### 生態系統

**Components:**
**組件：**

- Plants (vegetation, forests)
  植物（植被、森林）
- Animals (wildlife, livestock)
  動物（野生動物、牲畜）
- Predators
  捕食者
- Prey
  獵物
- Food chains
  食物鏈
- Climate effects
  氣候影響
- Water cycle
  水循環
- Resource cycles
  資源循環

**Emergent Causality Example:**
**湧現因果範例：**

```
Forest deforestation
↓
Soil erosion
↓
Agricultural decline
↓
Food price increase
↓
City population decline
↓
Political instability
↓
Revolution
```

### Weather and Climate
### 天氣與氣候

**Weather:**
**天氣：**

- Rain, snow, wind
  雨、雪、風
- Storms, hurricanes
  風暴、颶風
- Seasonal variations
  季節變化
- Daily cycles
  日常循環

**Climate:**
**氣候：**

- Climate zones
  氣候帶
- Long-term climate change
  長期氣候變遷
- Climate effects on civilization
  氣候對文明的影響
- Climate migration
  氣候遷移

---

## Civilization Simulation
## 文明模擬

### Civilization Components
### 文明組件

**Political:**
**政治：**

- Government type
  政府類型
- Laws and policies
  法律與政策
- Factions and parties
  派系與政黨
- Leadership
  領導
- Stability
  穩定性

**Economic:**
**經濟：**

- Trade
  貿易
- Markets
  市場
- Production
  生產
- Resources
  資源
- Currency
  貨幣
- Taxation
  稅收

**Military:**
**軍事：**

- Armies
  軍隊
- Weapons
  武器
- Fortifications
  防禦工事
- Logistics
  後勤
- Strategy
  策略

**Technology:**
**科技：**

- Research
  研究
- Innovation
  創新
- Knowledge
  知識
- Education
  教育

**Cultural:**
**文化：**

- Religion
  宗教
- Art
  藝術
- Values
  價值觀
- Traditions
  傳統
- Language
  語言

**Legal:**
**法律：**

- Laws
  法律
- Courts
  法院
- Justice system
  司法系統
- Enforcement
  執法

**Population:**
**人口：**

- Demographics
  人口統計
- Migration
  遷移
- Birth/death rates
  出生/死亡率
- Education
  教育

**Resources:**
**資源：**

- Natural resources
  自然資源
- Production capacity
  生產能力
- Infrastructure
  基礎設施
- Supply chains
  供應鏈

**Territory:**
**領土：**

- Land ownership
  土地所有權
- Borders
  邊界
- Colonies
  殖民地
- Claims
  主張

**Diplomacy:**
**外交：**

- Relations
  關係
- Treaties
  條約
- Alliances
  同盟
- Wars
  戰爭

**Intelligence:**
**情報：**

- Espionage
  間諜
- Counter-intelligence
  反間諜
- Information gathering
  資訊收集

**Social Classes:**
**社會階級：**

- Hierarchy
  階層
- Mobility
  流動性
- Privileges
  特權
- Tensions
  緊張

**Organizations:**
**組織：**

- Guilds
  公會
- Companies
  公司
- Religious orders
  宗教團體
- Secret societies
  秘密社會

**Cities:**
**城市：**

- Population
  人口
- Buildings
  建築
- Infrastructure
  基礎設施
- Economy
  經濟

**Infrastructure:**
**基礎設施：**

- Roads
  道路
- Ports
  港口
- Irrigation
  灌溉
- Fortifications
  防禦工事

### Civilization Dynamics
### 文明動態

**Rise:**
**興起：**

- Resource accumulation
  資源積累
- Population growth
  人口增長
- Technological advancement
  科技進步
- Military expansion
  軍事擴張

**Split:**
**分裂：**

- Internal conflict
  內部衝突
- Regional separation
  區域分離
- Cultural divergence
  文化分歧
- Political division
  政治分裂

**Merge:**
**合併：**

- Conquest
  征服
- Alliance
  同盟
- Federation
  聯邦
- Cultural assimilation
  文化同化

**Revolution:**
**革命：**

- Political overthrow
  政治推翻
- Regime change
  政權更迭
- Social upheaval
  社會動盪
- Ideological shift
  意識形態轉變

**Reform:**
**改革：**

- Policy change
  政策變更
- Institutional change
  制度變更
- Gradual evolution
  漸進演化
- Adaptation
  適應

**Expansion:**
**擴張：**

- Territorial growth
  領土增長
- Population growth
  人口增長
- Economic growth
  經濟增長
- Cultural spread
  文化傳播

**Decline:**
**衰退：**

- Resource depletion
  資源枯竭
- Population decline
  人口下降
- Technological stagnation
  科技停滯
- Military weakness
  軍事衰弱

**Destruction:**
**毀滅：**

- Conquest
  征服
- Collapse
  崩潰
- Catastrophe
  災難
- Extinction
  滅絕

**Revival:**
**復興：**

- Recovery
  恢復
- Rebirth
  重生
- Renaissance
  文藝復興
- New golden age
  新黃金時代

### Governance System
### 治理系統

**Governance Modes:**
**治理模式：**

- Conquest (military domination)
  征服（軍事支配）
- Deterrence (threat-based control)
  威懾（基於威脅的控制）
- Alliance (cooperative governance)
  同盟（合作治理）
- Trade (economic dominance)
  貿易（經濟支配）
- Diplomacy (political influence)
  外交（政治影響）
- Peace (non-violent coexistence)
  和平（非暴力共存）
- Federation (union of states)
  聯邦（國家聯盟）
- Empire (centralized control)
  帝國（中央集權）
- City-state (independent cities)
  城邦（獨立城市）
- Democracy (rule by people)
  民主（人民統治）
- Monarchy (rule by monarch)
  君主制（君主統治）
- Religious governance (theocracy)
  宗教治理（神權政治）
- AI governance (AI-led)
  AI 治理（AI 主導）
- Anarchy (no government)
  無政府（無政府）
- Mixed systems (combination)
  混合系統（組合）

**Non-Military Victory:**
**非軍事勝利：**

- "Victory without war"
  「不戰而勝」
- Enemies voluntarily join
  敵人自願加入
- Enemy civilizations convert to allies
  敵對文明轉為盟友
- Economic and cultural domination
  經濟與文化支配
- Deter enemy willingness to fight
  阻遏敵人戰意

**Multi-dimensional Victory:**
**多維勝利：**

- Survival
  生存
- Prosperity
  繁榮
- Peace
  和平
- Civilization continuity
  文明延續
- Cultural influence
  文化影響力
- Economic hegemony
  經濟霸權
- Technological hegemony
  科技霸權
- Military hegemony
  軍事霸權
- Diplomatic hegemony
  外交霸權
- Ecological balance
  生態平衡
- Population happiness
  人口幸福
- World unification
  世界統一
- World autonomy
  世界自治
- Transcending traditional win/loss
  超越傳統勝負

---

## Networking
## 網路

### Network Architecture
### 網路架構

**Client-Server Model:**
**客戶端-伺服器模型：**

- Authoritative server
  權威伺服器
- Rendering client
  渲染客戶端
- Prediction and reconciliation
  預測與協調

**Network Topologies:**
**網路拓撲：**

- Single Player (no networking)
  單人（無網路）
- Cooperative (shared world)
  合作（共享世界）
- MMO (massive multiplayer)
  MMO（大型多人）
- Persistent World (always-online)
  持久世界（始終在線）
- Dedicated Server (server-hosted)
  專用伺服器（伺服器託管）
- Cloud Simulation (distributed cloud)
  雲端模擬（分散式雲端）

### Server Responsibilities
### 伺服器職責

**World Simulation:**
**世界模擬：**

- Authoritative world state
  權威世界狀態
- AI execution
  AI 執行
- Physics simulation
  物理模擬
- Civilization simulation
  文明模擬
- Event processing
  事件處理

**State Synchronization:**
**狀態同步：**

- Replicate state to clients
  複製狀態到客戶端
- Delta compression
  增量壓縮
- Priority-based updates
  基於優先級的更新
- Bandwidth optimization
  頻寬優化

**Entity Replication:**
**實體複製：**

- Replicate relevant entities
  複製相關實體
- Entity creation/destruction
  實體建立/毀滅
- Component replication
  組件複製
- Property replication
  屬性複製

### Client Responsibilities
### 客戶端職責

**Rendering:**
**渲染：**

- Render world state
  渲染世界狀態
- Interpolate between updates
  在更新之間插值
- Extrapolate for prediction
  外推預測

**Prediction:**
**預測：**

- Client-side prediction
  客戶端預測
- Reconciliation with server
  與伺服器協調
- Lag compensation
  延遲補償

**Input:**
**輸入：**

- Send input to server
  發送輸入到伺服器
- Process local input immediately
  立即處理本地輸入

### Cloud / Distributed Simulation
### 雲端 / 分散式模擬

**World Sharding:**
**世界分片：**

- Split world into regions
  將世界分割為區域
- Each region simulated by separate worker
  每個區域由獨立工作者模擬
- Workers communicate via event bus
  工作者透過事件匯流排通訊

**Agent Migration:**
**代理遷移：**

- Agents move between regions
  代理在區域間移動
- State transfer between workers
  工作者間的狀態傳輸
- Seamless handoff
  無縫交接

**State Synchronization:**
**狀態同步：**

- Cross-region state sharing
  跨區域狀態共享
- Event propagation
  事件傳播
- Consistency guarantees
  一致性保證

**Event Bus:**
**事件匯流排：**

- Global event system
  全域事件系統
- Async event delivery
  非同步事件傳遞
- Event filtering and routing
  事件過濾與路由

**Distributed ECS:**
**分散式 ECS：**

- ECS spans multiple workers
  ECS 跨越多個工作者
- Distributed component storage
  分散式組件儲存
- Distributed system execution
  分散式系統執行

**Fault Recovery:**
**故障恢復：**

- Worker failure detection
  工作者故障檢測
- State recovery from snapshots
  從快照恢復狀態
- Load rebalancing
  負載重新平衡

---

## GPU Compute
## GPU 計算

### GPU Compute Scheduler
### GPU 計算排程器

**Compute Allocation:**
**計算分配：**

- Automatic distribution of compute tasks
  自動分配計算任務
- CPU vs GPU vs NPU vs Cloud
  CPU vs GPU vs NPU vs 雲端
- Dynamic load balancing
  動態負載平衡

**Task Types:**
**任務類型：**

- Rendering compute (shadows, post-processing)
  渲染計算（陰影、後處理）
- Physics compute (collision, dynamics)
  物理計算（碰撞、動力學）
- AI compute (inference, evaluation)
  AI 計算（推論、評估）
- Simulation compute (agents, particles)
  模擬計算（代理、粒子）
- Procedural generation (terrain, content)
  程序化生成（地形、內容）

### AI GPU Cluster
### AI GPU 叢集

**Compute Options:**
**計算選項：**

- Single GPU (local)
  單 GPU（本地）
- Multi GPU (local)
  多 GPU（本地）
- Local AI (local NPU/TPU)
  本地 AI（本地 NPU/TPU）
- Cloud AI (remote AI server)
  雲端 AI（遠端 AI 伺服器）

**Scheduler Decisions:**
**排程器決策：**

- Task complexity
  任務複雜度
- Resource availability
  資源可用性
- Latency requirements
  延遲要求
- Bandwidth constraints
  頻寬限制

---

## Data Architecture
## 資料架構

### Asset Pipeline
### 資產管線

**Import:**
**匯入：**

- Asset import from various formats
  從各種格式匯入資產
- Format validation
  格式驗證
- Metadata extraction
  元資料提取

**Processing:**
**處理：**

- Asset processing (compression, optimization)
  資產處理（壓縮、優化）
- LOD generation
  LOD 生成
- Format conversion
  格式轉換

**Cooking:**
**編譯：**

- Runtime format generation
  執行時格式生成
- Platform-specific optimization
  平台特定優化
- Bundling
  打包

**Storage:**
**儲存：**

- Asset database
  資產資料庫
- Version control
  版本控制
- Dependency tracking
  依賴追蹤

### Resource Management
### 資源管理

**Loading:**
**載入：**

- Synchronous loading
  同步載入
- Asynchronous loading
  非同步載入
- Streaming
  串流

**Streaming:**
**串流：**

- Geometry streaming
  幾何串流
- Texture streaming
  紋理串流
- Audio streaming
  音訊串流

**Caching:**
**快取：**

- Memory cache
  記憶體快取
- Disk cache
  磁碟快取
- GPU cache
  GPU 快取

### Serialization
### 序列化

**Save/Load:**
**儲存/載入：**

- World state serialization
  世界狀態序列化
- Component serialization
  組件序列化
- Entity serialization
  實體序列化
- Resource serialization
  資源序列化

**State Persistence:**
**狀態持久化：**

- Checkpoint system
  檢查點系統
- Incremental saves
  增量儲存
- Undo/redo
  復原/重做

### Database
### 資料庫

**Persistent Storage:**
**持久儲存：**

- Player data
  玩家資料
- World history
  世界歷史
- Analytics
  分析
- Telemetry
  遙測

**Database Types:**
**資料庫類型：**

- Document database (NoSQL)
  文件資料庫（NoSQL）
- Time-series database (history)
  時序資料庫（歷史）
- Graph database (relationships)
  圖資料庫（關係）

---

## Editor & Tools
## 編輯器與工具

### World Editor
### 世界編輯器

**Features:**
**功能：**

- Terrain editing
  地形編輯
- Vegetation placement
  植被放置
- City placement
  城市放置
- Resource placement
  資源放置
- Climate editing
  氣候編輯
- Biome editing
  生物群系編輯

### Civilization Editor
### 文明編輯器

**Features:**
**功能：**

- Civilization configuration
  文明配置
- Faction setup
  派系設置
- Political system setup
  政治系統設置
- Economic system setup
  經濟系統設置
- Military system setup
  軍事系統設置

### NPC Designer
### NPC 設計器

**Features:**
**功能：**

- NPC creation
  NPC 建立
- Personality configuration
  個性配置
- Memory setup
  記憶設置
- Relationship setup
  關係設置
- Goal configuration
  目標配置

### AI Behavior Designer
### AI 行為設計器

**Features:**
**功能：**

- AI behavior tree editing
  AI 行為樹編輯
- State machine editing
  狀態機編輯
- Parameter tuning
  參數調整
- Behavior visualization
  行為視覺化

### Agent Debugger
### 代理除錯器

**Features:**
**功能：**

- Agent thought inspection
  代理思維檢查
- Goal inspection
  目標檢查
- Memory inspection
  記憶檢查
- Knowledge inspection
  知識檢查
- Relationship inspection
  關係檢查
- Decision tree inspection
  決策樹檢查
- Utility score inspection
  效用分數檢查
- Current strategy inspection
  當前策略檢查

### World Inspector
### 世界檢查器

**Features:**
**功能：**

- World time inspection
  世界時間檢查
- Civilization inspection
  文明檢查
- Nation inspection
  國家檢查
- City inspection
  城市檢查
- War inspection
  戰爭檢查
- Economy inspection
  經濟檢查
- Population inspection
  人口檢查
- Ecology inspection
  生態檢查

### Timeline
### 時間軸

**Features:**
**功能：**

- History visualization
  歷史視覺化
- Time control (pause, play, fast-forward, rewind)
  時間控制（暫停、播放、快進、倒轉）
- Event filtering
  事件過濾
- Causality tracing
  因果追蹤

### Profiler
### 分析器

**Features:**
**功能：**

- Performance profiling
  效能分析
- CPU usage
  CPU 使用率
- GPU usage
  GPU 使用率
- Memory usage
  記憶體使用量
- Frame time
  幀時間
- System bottlenecks
  系統瓶頸
- Optimization suggestions
  優化建議

---

## Plugin System
## 外掛系統

### Plugin Architecture
### 外掛架構

**Plugin Types:**
**外掛類型：**

- Rendering plugins
  渲染外掛
- Physics plugins
  物理外掛
- AI plugins
  AI 外掛
- World generator plugins
  世界生成器外掛
- Gameplay system plugins
  遊戲玩法系統外掛
- UI plugins
  UI 外掛

**Plugin API:**
**外掛 API：**

- Standardized plugin interface
  標準化外掛介面
- Event hooks
  事件掛鉤
- Resource access
  資源存取
- System integration
  系統整合

**Hot Reload:**
**熱重載：**

- Plugin hot-reload
  外掛熱重載
- State preservation
  狀態保留
- Zero-downtime updates
  零停機更新

---

## Developer API
## 開發者 API

### Language Bindings
### 語言綁定

**C++ API:**
**C++ API：**

- Native, high-performance
  原生、高效能
- Full engine access
  完整引擎存取
- Low-level control
  低階控制

**Rust API:**
**Rust API：**

- Safe, systems programming
  安全、系統程式設計
- Memory safety
  記憶體安全
- Concurrency
  並行

**Python API:**
**Python API：**

- Scripting
  腳本
- AI integration
  AI 整合
- Data analysis
  資料分析

**TypeScript API:**
**TypeScript API：**

- Web integration
  網頁整合
- Tooling
  工具
- TypeScript safety
  TypeScript 安全

**Lua API:**
**Lua API：**

- Gameplay scripting
  遊戲玩法腳本
- Hot-reload
  熱重載
- Easy to learn
  易於學習

### Network APIs
### 網路 API

**REST API:**
**REST API：**

- Web services
  網頁服務
- HTTP-based
  基於 HTTP
- State queries
  狀態查詢

**WebSocket API:**
**WebSocket API：**

- Real-time communication
  即時通訊
- Event streaming
  事件串流
- Live updates
  即時更新

**RPC API:**
**RPC API：**

- Remote procedure calls
  遠端程序呼叫
- Direct function calls
  直接函式呼叫
- High performance
  高效能

---

## Mod System
## Mod 系統

### Modding Capabilities
### Mod 能力

**World Modding:**
**世界 Mod：**

- Modify world generation
  修改世界生成
- Custom geography
  自訂地理
- Custom climate
  自訂氣候

**Civilization Modding:**
**文明 Mod：**

- Custom civilizations
  自訂文明
- Custom political systems
  自訂政治系統
- Custom economic systems
  自訂經濟系統

**NPC Modding:**
**NPC Mod：**

- Custom NPC types
  自訂 NPC 類型
- Custom AI behaviors
  自訂 AI 行為
- Custom personalities
  自訂個性

**AI Modding:**
**AI Mod：**

- Custom AI systems
  自訂 AI 系統
- Custom decision logic
  自訂決策邏輯
- Custom memory systems
  自訂記憶系統

**Physics Modding:**
**物理 Mod：**

- Custom physics rules
  自訂物理規則
- Custom materials
  自訂材質
- Custom collision
  自訂碰撞

**UI Modding:**
**UI Mod：**

- Custom UI
  自訂 UI
- Custom HUD
  自訂 HUD
- Custom menus
  自訂選單

**Gameplay Modding:**
**遊戲玩法 Mod：**

- Custom rules
  自訂規則
- Custom mechanics
  自訂機制
- Custom win conditions
  自訂勝利條件

**Map Modding:**
**地圖 Mod：**

- Custom maps
  自訂地圖
- Custom scenarios
  自訂場景
- Custom campaigns
  自訂戰役

**Item Modding:**
**物品 Mod：**

- Custom items
  自訂物品
- Custom weapons
  自訂武器
- Custom equipment
  自訂裝備

**Quest Modding:**
**任務 Mod：**

- Custom quests
  自訂任務
- Custom storylines
  自訂故事線
- Custom events
  自訂事件

**Story Modding:**
**故事 Mod：**

- Custom narratives
  自訂敘事
- Custom lore
  自訂傳說
- Custom history
  自訂歷史

### Mod API
### Mod API

**Mod Interface:**
**Mod 介面：**

- Standardized mod API
  標準化 Mod API
- Sandbox for safety
  安全沙盒
- Resource access control
  資源存取控制

**Mod Loader:**
**Mod 載入器：**

- Mod discovery
  Mod 發現
- Mod loading
  Mod 載入
- Mod dependency resolution
  Mod 依賴解析
- Mod conflict detection
  Mod 衝突檢測

---

## Performance Targets
## 效能目標

### Primary Priority: Scale
### 主要優先級：規模

**Agent Simulation:**
**代理模擬：**

- 10,000 NPCs (Phase 6)
  10,000 NPC（第六階段）
- 100,000 NPCs (Phase 8)
  100,000 NPC（第八階段）
- 1,000,000 Agents (Phase 8)
  1,000,000 代理（第八階段）

**World Size:**
**世界大小：**

- Single map (Phase 1)
  單一地圖（第一階段）
- Regional world (Phase 6)
  區域世界（第六階段）
- Planetary world (Phase 13)
  行星世界（第十三階段）

**Player Count:**
**玩家數量：**

- Single player (Phase 1)
  單人（第一階段）
- Cooperative (Phase 9)
  合作（第九階段）
- MMO (Phase 9)
  MMO（第九階段）

### Secondary Priority: Determinism
### 次要優先級：確定性

**Deterministic Simulation:**
**確定性模擬：**

- Same inputs → same outputs
  相同輸入 → 相同輸出
- Reproducible results
  可重現結果
- Network synchronization
  網路同步

### Tertiary Priority: Modularity
### 第三優先級：模組化

**Modular Architecture:**
**模組化架構：**

- Pluggable systems
  可插拔系統
- Replaceable components
  可替換組件
- Clear interfaces
  清晰介面

### Quaternary Priority: Visual Fidelity
### 第四優先級：視覺保真度

**Rendering Quality:**
**渲染品質：**

- Indie quality (Phase 4)
  獨立品質（第四階段）
- AAA quality (Phase 13)
  AAA 品質（第十三階段）

### Quinary Priority: Developer Experience
### 第五優先級：開發者體驗

**Tooling:**
**工具：**

- Easy to use
  易於使用
- Well documented
  文件完善
- Fast iteration
  快速迭代

---

## Scalability Questions
## 可擴展性問題

**Every architecture decision must answer:**
**每個架構決策必須回答：**

1. If player count increases from 10,000 to 1,000,000, how does this system handle it?
   如果玩家數量從 10,000 增加到 1,000,000，此系統如何處理？
2. If NPC count increases from 10,000 to 1,000,000, how does this system handle it?
   如果 NPC 數量從 10,000 增加到 1,000,000，此系統如何處理？
3. If world size expands from one map to an entire planet, how does this system handle it?
   如果世界大小從一張地圖擴展到整個行星，此系統如何處理？

**Answers:**
**答案：**

- **Horizontal Scaling:** Distributed simulation, world sharding
  **水平擴展：** 分散式模擬、世界分片
- **Vertical Scaling:** LOD systems, importance sampling
  **垂直擴展：** LOD 系統、重要性採樣
- **Optimization:** GPU compute, parallel processing, cache optimization
  **優化：** GPU 計算、平行處理、快取優化
- **Architecture:** ECS, data-oriented design, event-driven
  **架構：** ECS、資料導向設計、事件驅動

---

## Engineering Principles
## 工程原則

### SOLID Principles
### SOLID 原則

- **Single Responsibility:** Each component/system has one reason to change
  **單一職責：** 每個組件/系統只有一個變更理由
- **Open/Closed:** Open for extension, closed for modification
  **開閉原則：** 對擴展開放，對修改關閉
- **Liskov Substitution:** Subtypes must be substitutable for base types
  **里氏替換：** 子類型必須可替換基類型
- **Interface Segregation:** Clients shouldn't depend on unused interfaces
  **介面隔離：** 客戶端不應依賴未使用的介面
- **Dependency Inversion:** Depend on abstractions, not concretions
  **依賴反轉：** 依賴抽象，不依賴具體

### Data-Oriented Design
### 資料導向設計

- Optimize for cache locality
  優化快取區域性
- Structure of Arrays (SoA) over Array of Structures (AoS)
  陣列結構 (SoA) 優於結構陣列 (AoS)
- SIMD-friendly data layouts
  SIMD 友善的資料佈局
- Batch processing
  批次處理

### ECS Principles
### ECS 原則

- Data-oriented entity management
  資料導向實體管理
- Component-based composition
  基於組件的組合
- System-based logic
  基於系統的邏輯
- Parallel execution
  平行執行

### Modular Architecture
### 模組化架構

- Clear module boundaries
  清晰模組邊界
- Well-defined interfaces
- 明確定義的介面
- Dependency injection
  依賴注入
- Plugin system
  外掛系統

### Deterministic Simulation
### 確定性模擬

- Fixed timestep
  固定時間步長
- Deterministic random number generation
  確定性隨機數生成
- Reproducible physics
  可重現物理
- Network synchronization
  網路同步

### Multithreading
### 多執行緒

- Thread-safe systems
  執行緒安全系統
- Lock-free data structures where possible
  盡可能使用無鎖資料結構
- Job system
  工作系統
- Parallel system execution
  平行系統執行

### GPU Compute
### GPU 計算

- GPU-accelerated systems
  GPU 加速系統
- Compute shaders
  計算著色器
- CUDA/OpenCL
  CUDA/OpenCL
- Tensor operations
  張量運算

### Memory Management
### 記憶體管理

- Memory pools
  記憶體池
- Custom allocators
  自訂分配器
- Cache-friendly allocation
  快取友善分配
- Zero-copy where possible
  盡可能零複製

### Async IO
### 非同步 IO

- Non-blocking I/O
  非阻塞 I/O
- Async file operations
  非同步檔案操作
- Async network operations
  非同步網路操作
- Event-driven
  事件驅動

### Event-Driven Architecture
### 事件驅動架構

- Event bus
  事件匯流排
- Event subscriptions
  事件訂閱
- Loose coupling
  鬆散耦合
- Reactive systems
  反應式系統

### Anti-Patterns to Avoid
### 避免的反模式

- **God Object:** Single class doing everything
  **上帝物件：** 單一類別做所有事情
- **God System:** Single system doing everything
  **上帝系統：** 單一系統做所有事情
- **Global State:** Mutable global state
  **全域狀態：** 可變全域狀態
- **Hidden Dependency:** Unclear dependencies
  **隱藏依賴：** 不明確的依賴
- **Hardcoded Gameplay:** Hardcoded game rules
  **硬編遊戲玩法：** 硬編遊戲規則
- **Spaghetti Architecture:** Tangled dependencies
  **義大利麵架構：** 糾纏的依賴

---

## Implementation Strategy
## 實作策略

### Vertical Slice → Expand → Optimize → Scale
### 垂直切片 → 擴展 → 優化 → 擴展規模

**Phase 0: Architecture**
**第 0 階段：架構**

- Design architecture
  設計架構
- Document systems
  文件化系統
- Plan roadmap
  規劃路線圖

**Phase 1: Core Engine**
**第 1 階段：核心引擎**

- Platform layer
  平台層
- Window, I/O
  視窗、I/O
- Basic rendering
  基本渲染
- Basic ECS
  基本 ECS
- 100 NPC simulation
  100 NPC 模擬

**Phase 2: ECS**
**第 2 階段：ECS**

- Full ECS implementation
  完整 ECS 實作
- Component system
  組件系統
- System scheduler
  系統排程器
- Parallel execution
  平行執行

**Phase 3: Rendering**
**第 3 階段：渲染**

- Full rendering pipeline
  完整渲染管線
- PBR, HDR, GI
  PBR、HDR、GI
- Post-processing
  後處理

**Phase 4: Physics**
**第 4 階段：物理**

- Physics engine
  物理引擎
- Collision detection
  碰撞檢測
- Rigid body dynamics
  剛體動力學

**Phase 5: AI Agent**
**第 5 階段：AI 代理**

- Agent runtime
  代理運行時
- Memory system
  記憶系統
- Decision system
  決策系統
- Basic AI
  基本 AI

**Phase 6: World Simulation**
**第 6 階段：世界模擬**

- World time system
  世界時間系統
- Procedural generation
  程序化生成
- Ecosystem
  生態系統
- 10,000 NPC simulation
  10,000 NPC 模擬

**Phase 7: Civilization Simulation**
**第 7 階段：文明模擬**

- Civilization systems
  文明系統
- Political, economic, military
  政治、經濟、軍事
- 10,000 NPC simulation
  10,000 NPC 模擬

**Phase 8: GPU Massive Simulation**
**第 8 階段：GPU 大規模模擬**

- GPU compute
  GPU 計算
- LOD systems
  LOD 系統
- 100,000 NPC simulation
  100,000 NPC 模擬
- 1,000,000 Agent simulation
  1,000,000 代理模擬

**Phase 9: Networking**
**第 9 階段：網路**

- Client-server
  客戶端-伺服器
- State synchronization
  狀態同步
- Distributed simulation
  分散式模擬

**Phase 10: Editor**
**第 10 階段：編輯器**

- World editor
  世界編輯器
- AI debugger
  AI 除錯器
- Profiler
  分析器

**Phase 11: Modding**
**第 11 階段：Modding**

- Mod system
  Mod 系統
- Mod API
  Mod API
- Plugin system
  外掛系統

**Phase 12: Cloud / Distributed Simulation**
**第 12 階段：雲端 / 分散式模擬**

- World sharding
  世界分片
- Agent migration
  代理遷移
- Fault recovery
  故障恢復

**Phase 13: AAA Production**
**第 13 階段：AAA 量產**

- AAA rendering
  AAA 渲染
- Full feature set
  完整功能集
- Optimization
  優化
- Polish
  打磨

---

## Verification and Testing
## 驗證與測試

### Self-Verification at Each Phase
### 每階段自我驗證

**Unit Tests:**
**單元測試：**

- Component testing
  組件測試
- System testing
  系統測試
- Module testing
  模組測試

**Integration Tests:**
**整合測試：**

- System integration
  系統整合
- Cross-module interaction
  跨模組互動
- End-to-end testing
  端到端測試

**Performance Tests:**
**效能測試：**

- FPS benchmark
  FPS 基準測試
- CPU usage
  CPU 使用率
- GPU usage
  GPU 使用率
- RAM usage
  RAM 使用量
- VRAM usage
  VRAM 使用量
- Simulation tick time
  模擬 tick 時間
- Agent count scaling
  代理數量擴展
- AI latency
  AI 延遲
- Network bandwidth
  網路頻寬

**Stress Tests:**
**壓力測試：**

- Maximum agent count
  最大代理數量
- Maximum entity count
  最大實體數量
- Maximum memory usage
  最大記憶體使用量
- Maximum network load
  最大網路負載

**Memory Tests:**
**記憶體測試：**

- Memory leaks
  記憶體洩漏
- Memory fragmentation
  記憶體碎片
- Memory pool efficiency
  記憶體池效率

**GPU Tests:**
**GPU 測試：**

- GPU memory usage
  GPU 記憶體使用量
- GPU compute performance
  GPU 計算效能
- GPU utilization
  GPU 利用率

**Determinism Tests:**
**確定性測試：**

- Reproducibility
  可重現性
- Network synchronization
  網路同步
- Save/load consistency
  儲存/載入一致性

**Network Tests:**
**網路測試：**

- Latency handling
  延遲處理
- Packet loss handling
  封包遺失處理
- Bandwidth optimization
  頻寬優化
- Connection stability
  連線穩定性

**AI Behavior Tests:**
**AI 行為測試：**

- Agent decision consistency
  代理決策一致性
- Agent learning
  代理學習
- Agent emergent behavior
  代理湧現行為

### Automated Benchmark Suite
### 自動化基準測試套件

**Metrics to Track:**
**追蹤指標：**

- FPS
  FPS
- CPU usage
  CPU 使用率
- GPU usage
  GPU 使用率
- RAM
  RAM
- VRAM
  VRAM
- Simulation tick time
  模擬 tick 時間
- Agent count
  代理數量
- AI latency
  AI 延遲
- Network bandwidth
  網路頻寬

**Comparison:**
**比較：**

- Before/after each change
  每次變更前後
- Regression detection
  回歸檢測
- Performance tracking
  效能追蹤

---

## Final Vision
## 最終願景

StonePotatoes will ultimately achieve:
StonePotatoes 將最終實現：

**A fully procedurally generated world with:**
**一個完全程序化生成的世界，具有：**

- Geography, climate, ecology, resources
  地理、氣候、生態、資源
- Civilization, economy, politics, religion, technology, history
  文明、經濟、政治、宗教、科技、歷史

**Where every significant character is an Autonomous Agent with:**
**其中每個重要角色都是一個自主代理，具有：**

- Identity, memory, personality, goals
  身份、記憶、個性、目標
- Ability to cooperate, compete, deceive, trade, war, negotiate, compromise, reform, and make peace
  合作、競爭、欺騙、貿易、戰爭、談判、妥協、改革與和平的能力

**World history is not pre-written.**
**世界歷史不是預先寫好的。**
**Stories are not pre-written.**
**故事不是預先寫好的。**
**Wars are not pre-written.**
**戰爭不是預先寫好的。**
**Civilizations are not pre-written.**
**文明不是預先寫好的。**

**Everything emerges from:**
**一切都從以下元素湧現：**

- Rules
  規則
- Agents
  代理
- Resources
  資源
- Environment
  環境
- Time
  時間

**Ultimately forming:**
**最終形成：**

- A Living World
  一個活著的世界
- A Living Civilization
  一個活著的文明
- A Living History
  一個活著的歷史
- A Living Game
  一個活著的遊戲

---

## No Fake Implementations
## 無假實作

**Strict Rule:**
**嚴格規則：**

- No fake APIs
  無假 API
- No fake ECS
  無假 ECS
- No fake AI
  無假 AI
- No fake GPU
  無假 GPU
- No fake physics
  無假物理
- No fake world simulation
  無假世界模擬
- No fake network synchronization
  無假網路同步
- No fake data
  無假資料

**All features must be:**
**所有功能必須：**

- Real Implementation
  真實實作

**If a feature is not yet complete:**
**如果功能尚未完成：**

- Explicitly mark as TODO
  明確標記為 TODO
- Explicitly mark as NOT IMPLEMENTED
  明確標記為 NOT IMPLEMENTED

**Never use fake code to pretend completion.**
**永遠不要使用假程式碼假裝完成。**

---

## Most Important Design Principle
## 最重要的設計原則

StonePotatoes should not be:
StonePotatoes 不應該是：
"A tool for developers to write games."
「開發者撰寫遊戲的工具。」

It should gradually become:
它應該逐漸成為：
"A machine for developers to create worlds."
「開發者創造世界的機器。」

Players are not executing scripts in a game.
玩家不是在遊戲中執行腳本。
Players are:
玩家是：
**Entering a world that is already running.**
**進入一個已經運行的世界。**

The world does not exist because of the player.
世界不是因為玩家而存在。
The world exists first.
世界先存在。
The player enters it.
玩家進入其中。
After the player leaves:
玩家離開後：
**The world continues to run.**
**世界繼續運行。**
