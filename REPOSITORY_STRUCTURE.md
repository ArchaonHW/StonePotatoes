# StonePotatoes Repository Structure
# StonePotatoes 儲存庫結構

## Overview | 概述

This document describes the repository structure for StonePotatoes. The structure is organized by modules and follows a clear separation of concerns.

本文檔描述 StonePotatoes 的儲存庫結構。該結構按模組組織，並遵循清晰的關注點分離原則。

---

## Root Directory Structure | 根目錄結構

```
StonePotatoes/
├── .devin/                      # Devin configuration and skills
│   # Devin 配置與技能
├── .github/                     # GitHub workflows and templates
│   # GitHub 工作流程與範本
├── docs/                        # Documentation
│   # 文件
├── src/                         # Source code
│   # 原始碼
├── tests/                       # Tests
│   # 測試
├── tools/                       # Build tools and scripts
│   # 建置工具與腳本
├── assets/                      # Engine assets (shaders, textures, models)
│   # 引擎資產（著色器、紋理、模型）
├── external/                    # Third-party dependencies
│   # 第三方相依項
├── build/                       # Build output (generated)
│   # 建置輸出（生成）
├── cmake/                       # CMake modules and scripts
│   # CMake 模組與腳本
├── scripts/                     # Utility scripts
│   # 實用腳本
├── examples/                    # Example projects
│   # 範例專案
├── .gitignore
├── CMakeLists.txt               # Main CMake build file
│   # 主要 CMake 建置檔案
├── ARCHITECTURE.md              # Architecture documentation
│   # 架構文件
├── ROADMAP.md                   # Technical roadmap
│   # 技術路線圖
├── REPOSITORY_STRUCTURE.md      # This file
│   # 本檔案
├── LICENSE                      # License file
│   # 授權檔案
└── README.md                    # Project README
    # 專案 README
```

---

## Directory Explanations | 目錄說明

### `.devin/`

**Purpose:** Devin AI agent configuration and custom skills.
**用途：** Devin AI 代理配置與自訂技能。

**Contents:**
- `config.json` - Devin configuration
- `skills/` - Custom skills for the repository

**內容：**
- `config.json` - Devin 配置
- `skills/` - 儲存庫的自訂技能

---

### `.github/`

**Purpose:** GitHub-specific configuration for CI/CD, issue templates, and project management.
**用途：** GitHub 特定配置，用於 CI/CD、問題範本與專案管理。

**Contents:**
- `workflows/` - GitHub Actions workflows
  - `ci.yml` - Continuous integration
  - `build.yml` - Build automation
  - `test.yml` - Test automation
  - `release.yml` - Release automation
- `ISSUE_TEMPLATE/` - Issue templates
- `PULL_REQUEST_TEMPLATE.md` - PR template
- `CODEOWNERS` - Code ownership

**內容：**
- `workflows/` - GitHub Actions 工作流程
  - `ci.yml` - 持續整合
  - `build.yml` - 建置自動化
  - `test.yml` - 測試自動化
  - `release.yml` - 發布自動化
- `ISSUE_TEMPLATE/` - 問題範本
- `PULL_REQUEST_TEMPLATE.md` - PR 範本
- `CODEOWNERS` - 程式碼擁有權

---

### `docs/`

**Purpose:** All project documentation.
**用途：** 所有專案文件。

**Contents:**
- `architecture/` - Detailed architecture documentation
  - `ecs.md` - ECS architecture
  - `rendering.md` - Rendering architecture
  - `physics.md` - Physics architecture
  - `ai.md` - AI architecture
  - `world.md` - World simulation architecture
  - `civilization.md` - Civilization simulation architecture
  - `networking.md` - Networking architecture
- `api/` - API documentation
  - `cpp/` - C++ API docs
  - `rust/` - Rust API docs
  - `python/` - Python API docs
  - `typescript/` - TypeScript API docs
  - `lua/` - Lua API docs
- `tutorials/` - Tutorial documentation
- `guides/` - User guides
- `design/` - Design documents
- `specifications/` - Technical specifications

**內容：**
- `architecture/` - 詳細架構文件
  - `ecs.md` - ECS 架構
  - `rendering.md` - 渲染架構
  - `physics.md` - 物理架構
  - `ai.md` - AI 架構
  - `world.md` - 世界模擬架構
  - `civilization.md` - 文明模擬架構
  - `networking.md` - 網路架構
- `api/` - API 文件
  - `cpp/` - C++ API 文件
  - `rust/` - Rust API 文件
  - `python/` - Python API 文件
  - `typescript/` - TypeScript API 文件
  - `lua/` - Lua API 文件
- `tutorials/` - 教學文件
- `guides/` - 使用者指南
- `design/` - 設計文件
- `specifications/` - 技術規格

---

### `src/`

**Purpose:** All source code organized by modules.
**用途：** 按模組組織的所有原始碼。

**Contents:**

```
src/
├── core/                        # Core engine systems
│   # 核心引擎系統
│   ├── platform/                # Platform abstraction (Window, IO, Threading)
│   │   # 平台抽象（視窗、IO、執行緒）
│   ├── memory/                  # Memory management (pools, allocators)
│   │   # 記憶體管理（記憶體池、分配器）
│   ├── time/                    # Time system
│   │   # 時間系統
│   ├── config/                  # Configuration system
│   │   # 配置系統
│   ├── logging/                 # Logging system
│   │   # 日誌系統
│   └── plugin/                  # Plugin system
│       # 外掛系統
├── ecs/                         # Entity-Component-System
│   # 實體-組件-系統
│   ├── entity/                  # Entity management
│   │   # 實體管理
│   ├── component/               # Component system
│   │   # 組件系統
│   ├── archetype/               # Archetype storage
│   │   # 原型儲存
│   ├── system/                  # System scheduler
│   │   # 系統排程器
│   ├── query/                   # Query system
│   │   # 查詢系統
│   ├── events/                  # Event system
│   │   # 事件系統
│   └── resources/               # Resource system
│       # 資源系統
├── rendering/                   # Rendering engine
│   # 渲染引擎
│   ├── rhi/                     # Render Hardware Interface
│   │   # 渲染硬體介面
│   │   ├── dx12/                # DirectX 12 backend
│   │   │   # DirectX 12 後端
│   │   ├── vulkan/              # Vulkan backend
│   │   │   # Vulkan 後端
│   │   └── metal/               # Metal backend
│   │       # Metal 後端
│   ├── shaders/                 # Shader system
│   │   # 著色器系統
│   ├── geometry/                # Geometry pipeline
│   │   # 幾何管線
│   ├── materials/               # Material system
│   │   # 材質系統
│   ├── pipeline/                # Rendering pipeline
│   │   # 渲染管線
│   ├── lighting/                # Lighting system
│   │   # 光照系統
│   ├── postprocessing/          # Post-processing
│   │   # 後處理
│   ├── ui/                      # UI rendering
│   │   # UI 渲染
│   └── debug/                   # Debug visualization
│       # 除錯視覺化
├── physics/                     # Physics engine
│   # 物理引擎
│   ├── core/                    # Physics core
│   │   # 物理核心
│   ├── collision/               # Collision detection
│   │   # 碰撞檢測
│   ├── dynamics/                # Rigid body dynamics
│   │   # 剛體動力學
│   ├── constraints/             # Constraints
│   │   # 約束
│   ├── character/               # Character controller
│   │   # 角色控制器
│   ├── softbody/                # Soft body physics
│   │   # 軟體物理
│   ├── fluid/                   # Fluid physics
│   │   # 流體物理
│   ├── destruction/             # Destruction physics
│   │   # 毀滅物理
│   └── gpu/                     # GPU physics
│       # GPU 物理
├── ai/                          # AI engine
│   # AI 引擎
│   ├── agent/                   # Agent runtime
│   │   # 代理運行時
│   ├── memory/                  # Memory system
│   │   # 記憶體系統
│   ├── decision/                # Decision system
│   │   # 決策系統
│   ├── perception/              # Perception system
│   │   # 感知系統
│   ├── behavior/                # Behavior trees
│   │   # 行為樹
│   ├── relationship/            # Relationship graph
│   │   # 關係圖
│   ├── personality/             # Personality system
│   │   # 個性系統
│   ├── learning/                # Learning system
│   │   # 學習系統
│   ├── hierarchy/               # AI hierarchy (Layers 0-5)
│   │   # AI 階層（第 0-5 層）
│   └── gpu/                     # GPU AI
│       # GPU AI
├── world/                       # World simulation
│   # 世界模擬
│   ├── time/                    # Multi-scale time
│   │   # 多尺度時間
│   ├── geography/               # Geography and terrain
│   │   # 地理與地形
│   ├── climate/                 # Climate and weather
│   │   # 氣候與天氣
│   ├── ecosystem/               # Ecosystem simulation
│   │   # 生態系統模擬
│   ├── procedural/              # Procedural generation
│   │   # 程序化生成
│   ├── history/                 # World history
│   │   # 世界歷史
│   └── resources/               # World resources
│       # 世界資源
├── civilization/                # Civilization simulation
│   # 文明模擬
│   ├── political/               # Political system
│   │   # 政治系統
│   ├── economic/                # Economic system
│   │   # 經濟系統
│   ├── military/                # Military system
│   │   # 軍事系統
│   ├── technology/              # Technology system
│   │   # 科技系統
│   ├── cultural/                # Cultural system
│   │   # 文化系統
│   ├── population/              # Population system
│   │   # 人口系統
│   ├── diplomatic/              # Diplomacy system
│   │   # 外交系統
│   ├── intelligence/            # Intelligence system
│   │   # 情報系統
│   ├── social/                  # Social system
│   │   # 社會系統
│   └── governance/              # Governance system
│       # 治理系統
├── agent/                       # Agent systems
│   # 代理系統
│   ├── generator/               # NPC generator
│   │   # NPC 生成器
│   ├── director/                # AI director
│   │   # AI 導演
│   ├── narrative/               # Narrative engine
│   │   # 敘事引擎
│   ├── quest/                   # Quest system
│   │   # 任務系統
│   ├── dialogue/                # Dialogue system
│   │   # 對話系統
│   └── reputation/              # Reputation system
│       # 聲譽系統
├── networking/                  # Networking
│   # 網路
│   ├── core/                    # Network core
│   │   # 網路核心
│   ├── server/                  # Server implementation
│   │   # 伺服器實作
│   ├── client/                  # Client implementation
│   │   # 客戶端實作
│   ├── replication/             # State replication
│   │   # 狀態複製
│   ├── rpc/                     # RPC system
│   │   # RPC 系統
│   ├── prediction/              # Prediction and reconciliation
│   │   # 預測與協調
│   ├── distributed/             # Distributed simulation
│   │   # 分散式模擬
│   └── cloud/                   # Cloud integration
│       # 雲端整合
├── data/                        # Data and assets
│   # 資料與資產
│   ├── pipeline/                # Asset pipeline
│   │   # 資產管線
│   ├── resources/               # Resource management
│   │   # 資源管理
│   ├── serialization/           # Serialization
│   │   # 序列化
│   ├── database/                # Database integration
│   │   # 資料庫整合
│   └── versioning/              # Asset versioning
│       # 資產版本控制
├── editor/                      # Editor and tools
│   # 編輯器與工具
│   ├── framework/               # Editor framework
│   │   # 編輯器框架
│   ├── world/                   # World editor
│   │   # 世界編輯器
│   ├── civilization/            # Civilization editor
│   │   # 文明編輯器
│   ├── npc/                     # NPC designer
│   │   # NPC 設計器
│   ├── ai/                      # AI behavior designer
│   │   # AI 行為設計器
│   ├── debugger/                # Agent debugger
│   │   # 代理除錯器
│   ├── inspector/               # World inspector
│   │   # 世界檢查器
│   ├── timeline/                # Timeline
│   │   # 時間軸
│   ├── profiler/                # Profiler
│   │   # 分析器
│   ├── asset_browser/           # Asset browser
│   │   # 資產瀏覽器
│   └── console/                 # Console
│       # 主控台
├── api/                         # Developer APIs
│   # 開發者 API
│   ├── cpp/                     # C++ API
│   ├── rust/                    # Rust API
│   ├── python/                  # Python API
│   ├── typescript/              # TypeScript API
│   ├── lua/                     # Lua API
│   ├── rest/                    # REST API
│   ├── websocket/               # WebSocket API
│   └── rpc/                     # RPC API
├── mod/                         # Mod system
│   # Mod 系統
│   ├── loader/                  # Mod loader
│   │   # Mod 載入器
│   ├── api/                     # Mod API
│   │   # Mod API
│   ├── sandbox/                 # Mod sandboxing
│   │   # Mod 沙盒
│   ├── hotreload/               # Mod hot-reload
│   │   # Mod 熱重載
│   └── tools/                   # Mod tools
│       # Mod 工具
├── gpu/                         # GPU compute
│   # GPU 計算
│   ├── framework/               # GPU compute framework
│   │   # GPU 計算框架
│   ├── particles/               # GPU particles
│   │   # GPU 粒子
│   ├── crowd/                   # GPU crowd simulation
│   │   # GPU 群眾模擬
│   ├── pathfinding/             # GPU pathfinding
│   │   # GPU 路徑規劃
│   └── inference/               # GPU AI inference
│       # GPU AI 推論
└── game/                        # Game engine API
    # 遊戲引擎 API
    ├── input/                   # Input handling
    │   # 輸入處理
    ├── audio/                   # Audio system
    │   # 音訊系統
    └── gameplay/                # Gameplay systems
        # 遊戲玩法系統
```

---

### `tests/`

**Purpose:** All test code organized by module.
**用途：** 按模組組織的所有測試程式碼。

**Contents:**

```
tests/
├── unit/                        # Unit tests
│   # 單元測試
│   ├── core/
│   ├── ecs/
│   ├── rendering/
│   ├── physics/
│   ├── ai/
│   ├── world/
│   ├── civilization/
│   ├── networking/
│   └── data/
├── integration/                # Integration tests
│   # 整合測試
│   ├── core/
│   ├── ecs/
│   ├── rendering/
│   ├── physics/
│   ├── ai/
│   ├── world/
│   ├── civilization/
│   └── networking/
├── performance/                 # Performance tests
│   # 效能測試
│   ├── ecs/
│   ├── rendering/
│   ├── physics/
│   ├── ai/
│   └── world/
├── stress/                      # Stress tests
│   # 壓力測試
│   ├── ecs/
│   ├── ai/
│   └── networking/
├── memory/                      # Memory tests
│   # 記憶體測試
│   ├── leaks/
│   └── fragmentation/
├── gpu/                         # GPU tests
│   # GPU 測試
├── determinism/                 # Determinism tests
│   # 確定性測試
└── network/                     # Network tests
    # 網路測試
```

---

### `tools/`

**Purpose:** Build tools, scripts, and utilities.
**用途：** 建置工具、腳本與實用程式。

**Contents:**

```
tools/
├── build/                       # Build tools
│   # 建置工具
│   ├── cmake/                   # CMake scripts
│   │   # CMake 腳本
│   └── premake/                 # Premake scripts (optional)
│       # Premake 腳本（可選）
├── asset/                       # Asset tools
│   # 資產工具
│   ├── importer/                # Asset importers
│   │   # 資產匯入器
│   ├── cooker/                  # Asset cooker
│   │   # 資產編譯器
│   ├── packer/                  # Asset packer
│   │   # 資產打包器
│   └── validator/               # Asset validator
│       # 資產驗證器
├── shader/                      # Shader tools
│   # 著色器工具
│   ├── compiler/                # Shader compiler
│   │   # 著色器編譯器
│   ├── optimizer/               # Shader optimizer
│   │   # 著色器優化器
│   └── reflection/              # Shader reflection
│       # 著色器反射
├── profiling/                   # Profiling tools
│   # 分析工具
│   ├── capture/                 # Profiling capture
│   │   # 分析擷取
│   ├── analysis/                # Profiling analysis
│   │   # 分析分析
│   └── visualization/          # Profiling visualization
│       # 分析視覺化
├── benchmark/                   # Benchmark tools
│   # 基準測試工具
│   ├── suite/                   # Benchmark suite
│   │   # 基準測試套件
│   └── comparison/              # Benchmark comparison
│       # 基準測試比較
├── deployment/                  # Deployment tools
│   # 部署工具
│   ├── installer/               # Installer generator
│   │   # 安裝程式產生器
│   └── updater/                 # Updater
│       # 更新程式
└── utilities/                   # General utilities
    # 一般實用程式
    ├── formatter/               # Code formatter
    │   # 程式碼格式化器
    ├── linter/                  # Code linter
    │   # 程式碼檢查器
    └── generator/               # Code generator
        # 程式碼產生器
```

---

### `assets/`

**Purpose:** Engine assets (shaders, textures, models, etc.).
**用途：** 引擎資產（著色器、紋理、模型等）。

**Contents:**

```
assets/
├── shaders/                     # Shaders
│   # 著色器
│   ├── hlsl/                    # HLSL shaders
│   │   # HLSL 著色器
│   ├── glsl/                    # GLSL shaders
│   │   # GLSL 著色器
│   └── spirv/                   # SPIR-V shaders (compiled)
│       # SPIR-V 著色器（已編譯）
├── textures/                    # Textures
│   # 紋理
│   ├── default/                 # Default textures
│   │   # 預設紋理
│   ├── ui/                      # UI textures
│   │   # UI 紋理
│   └── icons/                   # Icon textures
│       # 圖示紋理
├── models/                      # 3D models
│   # 3D 模型
│   ├── default/                 # Default models
│   │   # 預設模型
│   └── primitives/              # Primitive shapes
│       # 基本形狀
├── materials/                   # Materials
│   # 材質
│   └── default/                 # Default materials
│       # 預設材質
├── audio/                       # Audio assets
│   # 音訊資產
│   ├── sfx/                     # Sound effects
│   │   # 音效
│   └── music/                   # Music
│       # 音樂
├── fonts/                       # Fonts
│   # 字型
│   └── default/                 # Default fonts
│       # 預設字型
├── scripts/                     # Scripts
│   # 腳本
│   ├── lua/                     # Lua scripts
│   │   # Lua 腳本
│   └── python/                  # Python scripts
│       # Python 腳本
└── data/                        # Data files
    # 資料檔案
    ├── configs/                 # Configuration files
    │   # 配置檔案
    └── schemas/                 # Data schemas
        # 資料架構
```

---

### `external/`

**Purpose:** Third-party dependencies and libraries.
**用途：** 第三方相依項與函式庫。

**Contents:**

```
external/
├── fmt/                         # Format library
│   # 格式化函式庫
├── spdlog/                      # Logging library
│   # 日誌函式庫
├── glm/                         # Math library
│   # 數學函式庫
├── stb/                         # Stb libraries (image, etc.)
│   # Stb 函式庫（圖像等）
├── json/                        # JSON library
│   # JSON 函式庫
├── yaml/                        # YAML library
│   # YAML 函式庫
├── imgui/                       # Immediate mode GUI
│   # 即時模式 GUI
├── assimp/                      # Asset import library
│   # 資產匯入函式庫
├── vulkan/                      # Vulkan headers
│   # Vulkan 標頭檔
├── directx/                     # DirectX headers
│   # DirectX 標頭檔
├── openal/                      # OpenAL audio
│   # OpenAL 音訊
├── tensorflow/                  # TensorFlow (AI)
│   # TensorFlow (AI)
├── onnx/                        # ONNX Runtime (AI)
│   # ONNX Runtime (AI)
└── ...                          # Other dependencies
    # 其他相依項
```

---

### `build/`

**Purpose:** Build output directory (generated, not in version control).
**用途：** 建置輸出目錄（生成，不在版本控制中）。

**Contents:**

```
build/
├── Debug/                       # Debug build output
│   # Debug 建置輸出
├── Release/                     # Release build output
│   # Release 建置輸出
├── RelWithDebInfo/              # Release with debug info
│   # 帶除錯資訊的 Release
└── ...                          # Other build configurations
    # 其他建置配置
```

**Note:** This directory is gitignored.
**註：** 此目錄已加入 gitignore。

---

### `cmake/`

**Purpose:** CMake modules and scripts.
**用途：** CMake 模組與腳本。

**Contents:**

```
cmake/
├── modules/                     # CMake modules
│   # CMake 模組
│   ├── FindVulkan.cmake
│   ├── FindDirectX.cmake
│   └── ...
├── scripts/                     # CMake scripts
│   # CMake 腳本
│   ├── setup.cmake
│   └── ...
└── toolchains/                  # Toolchain files
    # 工具鏈檔案
    ├── windows-x64.cmake
    ├── linux-x64.cmake
    └── macos-x64.cmake
```

---

### `scripts/`

**Purpose:** Utility scripts for development.
**用途：** 開發用實用腳本。

**Contents:**

```
scripts/
├── setup/                       # Setup scripts
│   # 設定腳本
│   ├── setup_deps.sh
│   ├── setup_env.sh
│   └── setup_vs.bat
├── build/                       # Build scripts
│   # 建置腳本
│   ├── build.sh
│   ├── build.bat
│   └── clean.sh
├── test/                        # Test scripts
│   # 測試腳本
│   ├── run_tests.sh
│   ├── run_benchmarks.sh
│   └── run_stress_tests.sh
├── format/                      # Formatting scripts
│   # 格式化腳本
│   ├── format.sh
│   └── format.bat
├── lint/                        # Linting scripts
│   # 檢查腳本
│   ├── lint.sh
│   └── lint.bat
├── docs/                        # Documentation scripts
│   # 文件腳本
│   ├── generate_docs.sh
│   └── build_docs.sh
└── release/                     # Release scripts
    # 發布腳本
    ├── prepare_release.sh
    └── tag_release.sh
```

---

### `examples/`

**Purpose:** Example projects demonstrating engine usage.
**用途：** 展示引擎使用的範例專案。

**Contents:**

```
examples/
├── hello_world/                 # Basic hello world
│   # 基本 hello world
├── basic_rendering/             # Basic rendering example
│   # 基本渲染範例
├── physics_demo/                # Physics demo
│   # 物理展示
├── ai_demo/                     # AI demo
│   # AI 展示
├── world_demo/                  # World simulation demo
│   # 世界模擬展示
├── civilization_demo/           # Civilization demo
│   # 文明展示
├── multiplayer_demo/            # Multiplayer demo
│   # 多人遊戲展示
└── full_game/                   # Full game example
    # 完整遊戲範例
```

---

## File Naming Conventions | 檔案命名慣例

### C++ Files

- Headers: `PascalCase.h` (e.g., `Entity.h`, `TransformComponent.h`)
  標頭檔：`PascalCase.h`（例如：`Entity.h`、`TransformComponent.h`）
- Sources: `PascalCase.cpp` (e.g., `Entity.cpp`, `TransformComponent.cpp`)
  原始檔：`PascalCase.cpp`（例如：`Entity.cpp`、`TransformComponent.cpp`）
- Inline files: `PascalCase.inl` (e.g., `Entity.inl`)
  內聯檔：`PascalCase.inl`（例如：`Entity.inl`）

### Rust Files

- Modules: `snake_case.rs` (e.g., `entity.rs`, `transform_component.rs`)
  模組：`snake_case.rs`（例如：`entity.rs`、`transform_component.rs`）
- Crates: `kebab-case` (e.g., `stonepotatoes-core`, `stonepotatoes-ecs`)
  Crates：`kebab-case`（例如：`stonepotatoes-core`、`stonepotatoes-ecs`）

### Python Files

- Modules: `snake_case.py` (e.g., `entity.py`, `transform_component.py`)
  模組：`snake_case.py`（例如：`entity.py`、`transform_component.py`）
- Packages: `snake_case` (e.g., `stonepotatoes_core`, `stonepotatoes_ecs`)
  套件：`snake_case`（例如：`stonepotatoes_core`、`stonepotatoes_ecs`）

### TypeScript Files

- Modules: `PascalCase.ts` (e.g., `Entity.ts`, `TransformComponent.ts`)
  模組：`PascalCase.ts`（例如：`Entity.ts`、`TransformComponent.ts`）
- Packages: `kebab-case` (e.g., `stonepotatoes-core`, `stonepotatoes-ecs`)
  套件：`kebab-case`（例如：`stonepotatoes-core`、`stonepotatoes-ecs`）

### Lua Files

- Modules: `PascalCase.lua` (e.g., `Entity.lua`, `TransformComponent.lua`)
  模組：`PascalCase.lua`（例如：`Entity.lua`、`TransformComponent.lua`）

### Shader Files

- HLSL: `PascalCase.hlsl` (e.g., `VertexShader.hlsl`, `PixelShader.hlsl`)
  HLSL：`PascalCase.hlsl`（例如：`VertexShader.hlsl`、`PixelShader.hlsl`）
- GLSL: `snake_case.glsl` (e.g., `vertex_shader.glsl`, `pixel_shader.glsl`)
  GLSL：`snake_case.glsl`（例如：`vertex_shader.glsl`、`pixel_shader.glsl`）
- SPIR-V: `PascalCase.spv` (e.g., `VertexShader.spv`, `PixelShader.spv`)
  SPIR-V：`PascalCase.spv`（例如：`VertexShader.spv`、`PixelShader.spv`）

### Config Files

- JSON: `kebab-case.json` (e.g., `engine-config.json`)
  JSON：`kebab-case.json`（例如：`engine-config.json`）
- YAML: `kebab-case.yaml` (e.g., `engine-config.yaml`)
  YAML：`kebab-case.yaml`（例如：`engine-config.yaml`）
- TOML: `kebab-case.toml` (e.g., `engine-config.toml`)
  TOML：`kebab-case.toml`（例如：`engine-config.toml`）

---

## Module Dependencies | 模組相依性

### Core Layer | 核心層

```
core/ (no dependencies)
core/（無相依項）
```

### Engine Layer | 引擎層

```
ecs/ depends on: core/
ecs/ 相依於：core/
rendering/ depends on: core/, ecs/
rendering/ 相依於：core/、ecs/
physics/ depends on: core/, ecs/
physics/ 相依於：core/、ecs/
ai/ depends on: core/, ecs/
ai/ 相依於：core/、ecs/
world/ depends on: core/, ecs/
world/ 相依於：core/、ecs/
civilization/ depends on: core/, ecs/, world/
civilization/ 相依於：core/、ecs/、world/
agent/ depends on: core/, ecs/, ai/, civilization/
agent/ 相依於：core/、ecs/、ai/、civilization/
```

### Networking Layer | 網路層

```
networking/ depends on: core/, ecs/, world/, civilization/
networking/ 相依於：core/、ecs/、world/、civilization/
```

### Data Layer | 資料層

```
data/ depends on: core/, ecs/
data/ 相依於：core/、ecs/
```

### GPU Layer | GPU 層

```
gpu/ depends on: core/, ecs/, ai/, physics/
gpu/ 相依於：core/、ecs/、ai/、physics/
```

### Game Layer | 遊戲層

```
game/ depends on: core/, ecs/, rendering/, physics/, ai/
game/ 相依於：core/、ecs/、rendering/、physics/、ai/
```

### API Layer | API 層

```
api/ depends on: core/, ecs/, rendering/, physics/, ai/, world/, civilization/, game/
api/ 相依於：core/、ecs/、rendering/、physics/、ai/、world/、civilization/、game/
```

### Editor Layer | 編輯器層

```
editor/ depends on: core/, ecs/, rendering/, physics/, ai/, world/, civilization/, agent/, game/
editor/ 相依於：core/、ecs/、rendering/、physics/、ai/、world/、civilization/、agent/、game/
```

### Mod Layer | Mod 層

```
mod/ depends on: core/, ecs/, rendering/, physics/, ai/, world/, civilization/, agent/, game/
mod/ 相依於：core/、ecs/、rendering/、physics/、ai/、world/、civilization/、agent/、game/
```

---

## Build System | 建置系統

### CMake

The project uses CMake as the primary build system.
專案使用 CMake 作為主要建置系統。

**Main CMakeLists.txt** at root configures:
- Project settings
- C++ standard (C++20)
- Compiler flags
- Dependencies
- Subdirectories

**根目錄的 CMakeLists.txt** 配置：
- 專案設定
- C++ 標準（C++20）
- 編譯器旗標
- 相依項
- 子目錄

**Module CMakeLists.txt** in each module configures:
- Module sources
- Module headers
- Module dependencies
- Module libraries

**每個模組的 CMakeLists.txt** 配置：
- 模組原始碼
- 模組標頭檔
- 模組相依項
- 模組函式庫

### Build Configurations | 建置配置

- **Debug:** Full debug symbols, no optimization
  **Debug：** 完整除錯符號，無優化
- **Release:** Optimized, no debug symbols
  **Release：** 已優化，無除錯符號
- **RelWithDebInfo:** Optimized with debug symbols
  **RelWithDebInfo：** 帶除錯符號的優化
- **MinSizeRel:** Optimized for size
  **MinSizeRel：** 為大小優化

### Platform Support | 平台支援

- **Windows:** MSVC, MinGW
  **Windows：** MSVC、MinGW
- **Linux:** GCC, Clang
  **Linux：** GCC、Clang
- **macOS:** Clang
  **macOS：** Clang

---

## Version Control | 版本控制

### Git Flow

- `main` - Stable release branch
  `main` - 穩定發布分支
- `develop` - Development branch
  `develop` - 開發分支
- `feature/*` - Feature branches
  `feature/*` - 功能分支
- `bugfix/*` - Bug fix branches
  `bugfix/*` - 錯誤修復分支
- `release/*` - Release preparation branches
  `release/*` - 發布準備分支
- `hotfix/*` - Hotfix branches
  `hotfix/*` - 緊急修復分支

### Commit Conventions | 提交慣例

Follow [Conventional Commits](https://www.conventionalcommits.org/):
遵循 [Conventional Commits](https://www.conventionalcommits.org/)：

- `feat:` - New feature
  `feat:` - 新功能
- `fix:` - Bug fix
  `fix:` - 錯誤修復
- `docs:` - Documentation changes
  `docs:` - 文件變更
- `style:` - Code style changes (formatting)
  `style:` - 程式碼風格變更（格式化）
- `refactor:` - Code refactoring
  `refactor:` - 程式碼重構
- `perf:` - Performance improvements
  `perf:` - 效能改進
- `test:` - Test changes
  `test:` - 測試變更
- `chore:` - Build process, tools, etc.
  `chore:` - 建置程序、工具等

**Example:**
**範例：**
```
feat(ecs): add archetype-based component storage

Implement archetype-based storage for ECS components to improve
cache locality and enable SIMD operations.

Closes #123
```

---

## Documentation Standards | 文件標準

### Code Comments | 程式碼註解

- Use Doxygen-style comments for public APIs
  對公開 API 使用 Doxygen 風格註解
- Use inline comments for complex logic
  對複雜邏輯使用內聯註解
- Comment "why", not "what"
  註解「為什麼」，而非「什麼」

**Example:**
**範例：**
```cpp
/**
 * @brief Creates a new entity in the world.
 *
 * @param world The world to create the entity in.
 * @return Entity The created entity.
 */
Entity CreateEntity(World* world);
```

### README Standards | README 標準

Each module should have a README.md explaining:
- Module purpose
- Key components
- Usage examples
- Dependencies

每個模組應該有一個 README.md 說明：
- 模組用途
- 關鍵組件
- 使用範例
- 相依項

---

## Testing Standards | 測試標準

### Unit Tests | 單元測試

- Test individual components and functions
  測試個別組件與函式
- Mock dependencies
  模擬相依項
- Fast execution (< 1s per test)
  快速執行（每個測試 < 1 秒）

### Integration Tests | 整合測試

- Test module interactions
  測試模組互動
- Use real dependencies
  使用真實相依項
- Moderate execution (< 10s per test)
  中等執行時間（每個測試 < 10 秒）

### Performance Tests | 效能測試

- Benchmark critical paths
  對關鍵路徑進行基準測試
- Measure FPS, CPU, GPU, memory
  測量 FPS、CPU、GPU、記憶體
- Compare against baselines
  與基準比較

### Stress Tests | 壓力測試

- Test at scale (10k, 100k, 1M entities)
  大規模測試（1 萬、10 萬、100 萬實體）
- Test memory limits
  測試記憶體限制
- Test network limits
  測試網路限制

---

## Coding Standards | 編碼標準

### C++

- Use C++20 features
  使用 C++20 功能
- Follow Google C++ Style Guide
  遵循 Google C++ 風格指南
- Use RAII
  使用 RAII
- Prefer const correctness
  優先使用 const 正確性
- Use smart pointers (std::unique_ptr, std::shared_ptr)
  使用智慧指標（std::unique_ptr、std::shared_ptr）
- Avoid raw pointers where possible
  盡可能避免原始指標
- Use constexpr where possible
  盡可能使用 constexpr
- Use templates judiciously
  審慎使用範本

### Rust

- Use Rust 2021 edition
  使用 Rust 2021 版本
- Follow Rust API Guidelines
  遵循 Rust API 指南
- Use rustfmt
  使用 rustfmt
- Use clippy
  使用 clippy
- Prefer safe Rust over unsafe
  優先使用安全 Rust 而非 unsafe

### Python

- Use Python 3.10+
  使用 Python 3.10+
- Follow PEP 8
  遵循 PEP 8
- Use type hints
  使用型別提示
- Use black formatter
  使用 black 格式化器
- Use pylint
  使用 pylint

### TypeScript

- Use TypeScript 5+
  使用 TypeScript 5+
- Follow ESLint rules
  遵循 ESLint 規則
- Use Prettier formatter
  使用 Prettier 格式化器
- Use strict mode
  使用嚴格模式

---

## Security Standards | 安全性標準

- No secrets in code
  程式碼中無密碼
- No hardcoded credentials
  無硬編碼憑證
- Input validation
  輸入驗證
- Output encoding
  輸出編碼
- Secure defaults
  安全預設值
- Regular dependency updates
  定期更新相依項
- Security audits
  安全稽核

---

## Performance Standards | 效能標準

- Profile before optimizing
  優化前先分析
- Optimize hot paths
  優化熱路徑
- Use cache-friendly data structures
  使用快取友善的資料結構
- Minimize allocations
  最小化分配
- Use object pools
  使用物件池
- Avoid premature optimization
  避免過早優化
- Measure everything
  測量一切

---

## Summary | 總結

This repository structure provides:
- Clear module organization
- Separation of concerns
- Scalable architecture
- Easy navigation
- Consistent naming
- Proper dependencies
- Comprehensive testing
- Complete documentation

此儲存庫結構提供：
- 清晰的模組組織
- 關注點分離
- 可擴展架構
- 易於導航
- 一致的命名
- 適當的相依性
- 全面的測試
- 完整的文件

The structure is designed to support the long-term development of StonePotatoes from Phase 0 through Phase 13 and beyond.
該結構設計為支援 StonePotatoes 從第 0 階段到第 13 階段及以後的長期開發。
