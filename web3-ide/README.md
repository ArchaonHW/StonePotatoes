# Web3.0 AI Agent IDE - Spring Boot + Cloud Architecture

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      Web3.0 AI Agent IDE                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ Web UI (Vue) │  │ VS Code Ext  │  │ Desktop App  │          │
│  │              │  │              │  │   (JavaFX)   │          │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘          │
│         │                 │                 │                   │
│         └─────────────────┴─────────────────┘                   │
│                           │                                       │
│                    ┌─────▼─────┐                                 │
│                    │  Gateway  │ (Spring Cloud Gateway)       │
│                    └─────┬─────┘                                 │
│                          │                                       │
│    ┌─────────────────────┼─────────────────────┐              │
│    │                     │                     │              │
│  ┌─▼────┐          ┌─────▼────┐          ┌─────▼────┐         │
│  │Agent  │          │ Config   │          │ Eureka   │         │
│  │Service│◄────────►│  Server  │◄────────►│ Registry │         │
│  │(Spring│          │          │          │          │         │
│  │ Boot) │          └──────────┘          └──────────┘         │
│  └───┬───┘                                                   │
│      │                                                        │
│      ├──► Anvil (Local Chain)                                 │
│      ├──► llama.cpp (Local LLM)                               │
│      └──► Kafka (P2P Events)                                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Modules

### 1. eureka-server (Port 8761)
Service discovery registry for microservices.

### 2. config-server (Port 8888)
Centralized configuration management backed by Git.

### 3. agent-service (Port 8081)
Core Web3 Agent microservice - Spring Boot wrapper around existing agent logic.
- REST API for agent interactions
- WebSocket support for real-time events
- Integration with Anvil and llama.cpp
- P2P networking via Kafka

### 4. gateway (Port 8080)
API Gateway routing to all services.

### 5. web-ui (Port 3000)
Vue.js frontend for web-based IDE.

## Prerequisites

- JDK 25
- Maven 3.9.11
- Docker (for Kafka/Zookeeper if needed)
- Anvil (local chain)
- llama.cpp (local LLM)

## Quick Start

### 1. Start Infrastructure

```bash
# Start Eureka Server
cd eureka-server
mvn spring-boot:run

# Start Config Server
cd config-server
mvn spring-boot:run

# Start Kafka (optional, for P2P events)
docker-compose up -f docker-compose.kafka.yml
```

### 2. Start Anvil and llama.cpp

```bash
# Anvil
/d/HWC/.tools/foundry/anvil.exe --host 127.0.0.1 --port 8545 --chain-id 31337

# llama.cpp
cd /d/HWC/.tools/llama
./llama-server.exe --model /d/HWC/.tools/_dl/qwen2.5-3b-instruct-q4_k_m.gguf --host 127.0.0.1 --port 11434 --ctx-size 4096 --n-gpu-layers 99
```

### 3. Start Agent Service

```bash
cd agent-service
export JAVA_HOME=/d/HWC/.tools/jdk/jdk-25.0.4.1+1
export PATH=$JAVA_HOME/bin:/d/HWC/.tools/apache-maven-3.9.11/bin:$PATH
export WEB3AGENT_PASSWORD=test123
mvn spring-boot:run
```

### 4. Start Gateway

```bash
cd gateway
mvn spring-boot:run
```

### 5. Test API

```bash
# Agent info
curl http://localhost:8080/api/agent/info

# Chat with agent
curl -X POST http://localhost:8080/api/agent/chat \
  -H "Content-Type: application/json" \
  -d '{"message":"What is my address?"}'
```

## API Endpoints

### Agent Service (via Gateway on port 8080)

- `GET /api/agent/info` - Agent information
- `POST /api/agent/chat` - Chat with agent
- `GET /api/agent/balance?address=0x...` - Get balance

## Configuration

Configuration is managed by Config Server. Create a Git repo at `~/web3-ide-config` with:

```
web3-ide-config/
├── agent-service.yml
├── gateway.yml
└── eureka-server.yml
```

Example `agent-service.yml`:
```yaml
agent:
  chain:
    url: http://127.0.0.1:8545
  llm:
    type: openai
    url: http://127.0.0.1:11434/v1
    model: qwen2.5:7b
```

## Next Steps

- [ ] Implement Web UI (Vue.js)
- [ ] Implement VS Code Extension
- [ ] Implement Desktop App (JavaFX)
- [ ] Add Kafka integration for P2P events
- [ ] Add WebSocket support for real-time updates
- [ ] Add authentication and authorization
- [ ] Add monitoring and logging (ELK stack)
