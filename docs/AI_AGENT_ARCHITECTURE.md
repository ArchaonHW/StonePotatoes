# StonePotatoes Local-First Native AI Agent Platform

## 架構概覽

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     Spring Cloud Infrastructure                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │ Eureka   │  │ Config   │  │ Gateway  │  │  Kafka   │             │
│  │ Server   │  │ Server   │  │          │  │  Stream  │             │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘             │
│       │            │            │            │                     │
│       └────────────┴────────────┴────────────┘                     │
│                      Service Mesh / Orchestration                   │
└────────────────────────────────────┬────────────────────────────────┘
                                     │
                  ┌──────────────────┼──────────────────┐
                  │                  │                  │
         ┌────────▼────────┐  ┌─────▼──────┐  ┌─────▼──────┐
         │  Agent Service  │  │  Agent Pod  │  │  Agent Pod  │
         │  (Spring Boot)  │  │  (Native)   │  │  (Native)   │
         └────────┬─────────┘  └─────┬──────┘  └─────┬──────┘
                  │                  │                  │
         ┌────────▼────────────────────────▼────────────────▼──────┐
         │         Local Agent Runtime (Native C++/Rust)           │
         │  ┌─────────────┐  ┌──────────┐  ┌──────────────────┐  │
         │  │  Agent Brain│  │  Memory  │  │   Tool Runtime   │  │
         │  │             │  │  System  │  │                  │  │
         │  │ Planning    │  │          │  │ • Game Engine    │  │
         │  │ Reasoning   │  │          │  │ • Vulkan/CUDA    │  │
         │  │ Execution   │  │          │  │ • File System    │  │
         │  │ Reflection  │  │          │  │ • Network P2P    │  │
         │  │ Learning    │  │          │  │ • Blockchain     │  │
         │  └──────┬──────┘  └────┬─────┘  └──────────────────┘  │
         └─────────┼──────────────┼──────────────┼───────────────┘
                   │              │              │
         ┌─────────▼──────────────▼──────────────▼───────────────┐
         │              Local-First Resources                     │
         │  ┌──────────┐  ┌──────────┐  ┌──────────┐           │
         │  │  CPU     │  │   GPU    │  │   NPU    │           │
         │  └──────────┘  └──────────┘  └──────────┘           │
         │  ┌──────────┐  ┌──────────┐  ┌──────────┐           │
         │  │Local LLM │  │ Local    │  │ Local    │           │
         │  │          │  │Embedding │  │Vector DB │           │
         │  └──────────┘  └──────────┘  └──────────┘           │
         └──────────────────────────────────────────────────────┘
```

## 核心設計原則

### 1. Local First

**優先使用本地資源：**
- CPU：本地計算能力
- GPU/NPU：本地加速（Vulkan/CUDA/ROCm）
- Local LLM：llama.cpp / ggml / 其他本地推理引擎
- Local Embedding：本地文本嵌入模型
- Local Vector Database：本地向量存儲（Chroma / Weaviate / Qdrant）
- Local Memory：本地記憶系統（短期/長期/工作記憶）
- Local Tool Runtime：本地工具執行環境

**Cloud AI 只能作為：**
- Optional：可選擇性啟用
- Fallback：本地資源不足時的備選
- Research：實驗性功能
- Large Model Escalation：超大型模型調用

### 2. Agent Brain Architecture

```
Agent Brain
├── Planning
│   ├── Goal Decomposition
│   ├── Task Sequencing
│   └── Resource Allocation
├── Reasoning
│   ├── Logical Inference
│   ├── Contextual Understanding
│   └── Multi-step Problem Solving
├── Execution
│   ├── Tool Invocation
│   ├── Action Monitoring
│   └── Error Recovery
├── Reflection
│   ├── Self-evaluation
│   ├── Performance Analysis
│   └── Strategy Adjustment
└── Learning
    ├── Experience Replay
    ├── Model Fine-tuning
    └── Knowledge Base Update
```

### 3. Memory System

```
Memory System
├── Working Memory
│   ├── Current Context
│   ├── Active Tasks
│   └── Short-term State
├── Episodic Memory
│   ├── Past Experiences
│   ├── Success/Failure Patterns
│   └── Temporal Associations
├── Semantic Memory
│   ├── Domain Knowledge
│   ├── World Model
│   └── Tool Capabilities
└── Procedural Memory
    ├── Tool Usage Patterns
    ├── Task Execution Strategies
    └── Optimization Heuristics
```

### 4. Tool Runtime

```
Tool Runtime
├── Game Engine Integration
│   ├── Entity State Query
│   ├── World State Modification
│   ├── Physics Simulation
│   └── Rendering Control
├── GPU Acceleration
│   ├── Vulkan Compute Shaders
│   ├── CUDA (if available)
│   └── Tensor Operations
├── File System
│   ├── Configuration
│   ├── Data Persistence
│   └── Log Management
├── Network P2P
│   ├── Agent Discovery
│   ├── Message Passing
│   └── Distributed Coordination
└── Blockchain
    ├── Smart Contract Interaction
    ├── Transaction Execution
    └── Asset Management
```

## Spring Cloud 整合

### 服務架構

```
StonePotatoes AI IDE
├── eureka-server (8761)
│   └── Service Discovery
├── config-server (8888)
│   └── Centralized Configuration
├── gateway (8088)
│   └── API Gateway & Load Balancer
├── agent-service (8092)
│   ├── Agent Lifecycle Management
│   ├── HTTP/REST Interface
│   └── WebSocket/Streaming
├── memory-service (待實現)
│   ├── Distributed Memory
│   └── Vector Database Integration
├── tool-service (待實現)
│   ├── Tool Registry
│   └── Execution Monitoring
└── orchestration-service (待實現)
    ├── Agent Coordination
    └── Task Scheduling
```

### 可擴展性

- **水平擴展**：多個 Agent Pod 實例
- **垂直擴展**：單個實例資源調整
- **混合部署**：本地 + 雲端實例
- **地域分佈**：跨地域 Agent 協作

### 可治理性

- **配置管理**：Config Server 集中配置
- **服務治理**：Eureka 服務註冊與發現
- **路由管理**：Gateway 動態路由
- **限流熔斷**：Resilience4j 保護
- **監控告警**：Prometheus + Grafana

### 可觀測性

- **日誌聚合**：ELK Stack / Loki
- **指標監控**：Micrometer + Prometheus
- **分散式追蹤**：Sleuth + Zipkin
- **性能分析**：APM 工具集成

### 可分散式部署

- **服務發現**：Eureka 自動註冊
- **負載均衡**：Ribbon / Spring Cloud LoadBalancer
- **配置同步**：Config Server Git 後端
- **事件驅動**：Kafka 事件流
- **狀態同步**：分佈式協議

## 實現路徑

### 階段 1：基礎架構（當前進度）
- ✅ web3-agent 本地 Agent 實現
- ✅ Spring Boot + Cloud 微服務骨架
- ✅ Eureka、Config Server、Gateway
- ✅ HTTP 服務橋接
- ✅ llama.cpp 本地 LLM 集成

### 階段 2：Agent Brain
- [ ] Planning 模組
- [ ] Reasoning 模組
- [ ] Execution 模組
- [ ] Reflection 模組
- [ ] Learning 模組

### 階段 3：Memory System
- [ ] Working Memory 實現
- [ ] Episodic Memory 實現
- [ ] Semantic Memory 實現
- [ ] Procedural Memory 實現
- [ ] Local Vector Database 集成

### 階段 4：Tool Runtime
- [ ] Game Engine C++ 集成
- [ ] Vulkan GPU 加速
- [ ] P2P 網絡協議
- [ ] Blockchain 工具

### 階段 5：Spring Cloud 深度整合
- [ ] Memory Service (分佈式記憶)
- [ ] Tool Service (工具註冊與監控)
- [ ] Orchestration Service (協調與調度)
- [ ] Kafka 事件流
- [ ] WebSocket 實時通信

### 階段 6：高級特性
- [ ] Agent 自我進化
- [ ] 多 Agent 協作
- [ ] 跨 Agent 學儲
- [ ] 雲端 Fallback 機制
- [ ] 安全與隔離

## 技術棧

### Native Agent Runtime
- **語言**：C++ / Rust
- **LLM 引擎**：llama.cpp / ggml
- **嵌入引擎**：sentence-transformers / local
- **向量數據庫**：Chroma / Weaviate / Qdrant
- **GPU 加速**：Vulkan / CUDA / ROCm
- **網絡**：libp2p / 自研 P2P 協議
- **區塊鏈**：web3j / ethers.js

### Spring Cloud Infrastructure
- **服務框架**：Spring Boot 3.3+
- **服務治理**：Spring Cloud 2023.0+
- **服務發現**：Eureka
- **配置中心**：Spring Cloud Config
- **API 網關**：Spring Cloud Gateway
- **事件流**：Spring Cloud Stream + Kafka
- **監控**：Micrometer + Prometheus
- **追蹤**：Spring Cloud Sleuth

### Web UI
- **框架**：Vue.js 3 + Element Plus
- **構建**：Vite
- **實時通信**：WebSocket / SSE
- **狀態管理**：Pinia
- **圖表**：ECharts / D3.js

## 當前狀態

✅ **已完成：**
- web3-agent 本地 Agent 實現（錢包、鏈上、P2P、LLM）
- Spring Boot + Cloud 微服務骨架
- Eureka、Config Server、Gateway
- HTTP 服務橋接（web3-agent → Agent Service）
- llama.cpp 本地 LLM 集成
- Chat 端點修復並工作

⚠️ **待完成：**
- Web UI npm 安裝（權限問題）
- Kafka 事件流集成
- Agent Brain 各模組實現
- Memory System 實現
- Game Engine 集成
- 高級特性（自我進化、多 Agent 協作）
