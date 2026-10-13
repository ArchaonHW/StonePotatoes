# StonePotatoes Web3 Local AI Agent

A self-sovereign AI agent that runs entirely on your machine:

- **Identity / wallet** – secp256k1 key in an encrypted Ethereum V3 keystore. The key never leaves `AgentWallet`; it only produces signatures.
- **On-chain** – EVM JSON-RPC (Anvil / Hardhat / Geth): chain info, ETH balance, ERC-20 `balanceOf`, signed EIP-1559 ETH transfers, contract calls, ERC-20 transfers, contract deployment (built-in AgentToken).
- **Peer-to-peer** – serverless TCP gossip between agents. Every message is an EIP-191 signed envelope; peer identity = recovered Ethereum address. Forged, replayed, stale (>5 min) and oversized messages are dropped.
- **Content-addressed storage** – blobs keyed by `keccak256(content)`, self-verifying, replicable to peers (`store_share`).
- **Pluggable LLM** – offline rule-based `mock` (default) or any OpenAI-compatible endpoint (Ollama, llama.cpp, LM Studio, vLLM).

## Build & test

Toolchain lives in `D:\HWC\.tools` (JDK 25 + Maven 3.9.11):

```bash
export JAVA_HOME=/d/HWC/.tools/jdk/jdk-25.0.4.1+1
export PATH=$JAVA_HOME/bin:/d/HWC/.tools/apache-maven-3.9.11/bin:$PATH
cd web3-agent
mvn test                    # 15 offline tests (fake EVM node, gossip, agent loop)
ANVIL_TEST=1 mvn test       # +3 integration tests against real Anvil
LLM_TEST=1 mvn test         # +1 integration test against real llama.cpp
mvn package                 # -> target/web3-agent-0.1.0.jar (fat jar)
```

## Run

```bash
# optional local chain (Foundry): anvil   -> http://127.0.0.1:8545, chainId 31337
export WEB3AGENT_PASSWORD='choose-a-strong-password'
java -jar target/web3-agent-0.1.0.jar --port 30401

# second agent, joining the first
java -jar target/web3-agent-0.1.0.jar --data-dir ./agent2 --port 30402 --peer 127.0.0.1:30401

# real local LLM via llama.cpp (Qwen 2.5 3B Q4_K_M)
cd /d/HWC/.tools/llama
./llama-server.exe --model /d/HWC/.tools/_dl/qwen2.5-3b-instruct-q4_k_m.gguf --host 127.0.0.1 --port 11434 --ctx-size 4096 --n-gpu-layers 99
java -jar target/web3-agent-0.1.0.jar --llm openai --llm-url http://127.0.0.1:11434/v1 --llm-model qwen2.5:7b
```

`--help` lists all options. Example requests for the mock LLM: `address`, `balance`, `chain info`,
`send 0.01 eth to 0x…`, `token 0x<erc20>`, `sign hello`, `broadcast gm`, `peers`, `inbox`, `store some text`, `<cid>`, `share <cid>`, `call 0x<contract> signature(in)(out)`.

A fresh wallet on Anvil has no ETH; fund it from an Anvil dev account, e.g.
`cast send <agent-address> --value 1ether --private-key <anvil-key-0>`.

## New tools (v0.2)

- `call_contract` – read-only ABI call, e.g. `call 0xToken balanceOf(address)(uint256) [owner]`
- `contract_tx` – state-changing contract call, requires approval
- `erc20_transfer` – transfer ERC-20 tokens in human units, requires approval
- `deploy_token` – deploy the built-in AgentToken ERC-20, requires approval
- `receipt` – fetch transaction receipt by hash
- `wait_receipt` – wait up to 60s for receipt

## Safety model

- LLM output is treated as untrusted. Transfers go through `SpendPolicy`: per-tx limit (`--max-tx-eth`, default 0.1), daily limit (`--max-daily-eth`, default 1) **and** an interactive `[y/N]` confirmation for every transfer.
- Peer messages and stored blobs are marked as untrusted data in the system prompt (prompt-injection mitigation); the spend policy is the hard guard regardless.
- P2P binds to `127.0.0.1` by default; use `--bind 0.0.0.0` only on trusted networks (traffic is signed, not encrypted).
- Keystore uses full scrypt. Default data dir: `~/.stonepotatoes/web3-agent`.

## Layout

```
src/main/java/io/stonepotatoes/web3agent/
  wallet/AgentWallet        keystore, EIP-191 sign/recover, tx signing
  chain/ChainClient         EVM JSON-RPC (web3j 6), EIP-1559, receipts, ABI
  chain/SpendPolicy         limits + approval guard
  chain/AbiSignature        human-writable ABI signatures (cast-style)
  p2p/SignedEnvelope        signed gossip message
  p2p/PeerNode              TCP gossip node (virtual threads)
  p2p/ContentStore          keccak256 content-addressed blobs
  llm/                      LlmProvider, MockLlmProvider, OpenAiCompatibleProvider
  agent/Web3Agent           JSON tool-call loop
  agent/Web3Tools           tool bindings
  cli/Main                  REPL entry point
contracts/                 Solidity contracts (AgentToken.sol, forge built)
```

## Real environment setup

### Foundry (anvil)

```bash
# Foundry 1.8.5 is installed at D:\HWC\.tools\foundry
/d/HWC/.tools/foundry/anvil.exe --host 127.0.0.1 --port 8545 --chain-id 31337 --block-time 12
```

### llama.cpp (Qwen 2.5 3B Q4_K_M)

```bash
# llama.cpp v0.6.0 (build b11430) is installed at D:\HWC\.tools\llama
# Qwen 2.5 3B Q4_K_M is downloaded to D:\HWC\.tools\_dl\qwen2.5-3b-instruct-q4_k_m.gguf
cd /d/HWC/.tools/llama
./llama-server.exe --model /d/HWC/.tools/_dl/qwen2.5-3b-instruct-q4_k_m.gguf --host 127.0.0.1 --port 11434 --ctx-size 4096 --n-gpu-layers 99
```
