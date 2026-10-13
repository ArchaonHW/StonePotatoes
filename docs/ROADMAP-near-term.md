# StonePotatoes 近期路線圖（v0.1 → v0.3）

> 本文件是**可執行的近期計畫**，以 GitHub issues #3–#15 與里程碑為單位追蹤。
> 長期 14 階段願景（Phase 0–13）仍保留於根目錄 [`ROADMAP.md`](../ROADMAP.md)。
> 最後更新：2026-10-14（台北時間）

## 0. 現況快照

| 區塊 | 位置 | 狀態 | 測試／CI |
|---|---|---|---|
| C++ 引擎核心（core / ecs / math / iqgpu / gpu / ai / world） | `src/` | 可運作；ECS、wave 平行排程、IQGPU CPU 後端、Vulkan 計算後端、NPC 垂直切片、程序化世界串流 | `ctest` 6 項（Vulkan 無 GPU 時 Skipped）；src 行覆蓋率約 77.5% |
| C++ 尚未實作模組 | `src/{game,data,rendering,physics,agent,civilization,networking,editor,api/*}` | 只有 `stub.cpp` | — |
| self_developing_agent | `src/self_developing_agent/` | 實驗性；`src/main.cpp` demo 使用 | 無專屬測試 |
| web3-agent（Java 25） | `web3-agent/` | 可運作；EIP-1559、合約、P2P、LLM | `mvn test`（離線）+ 可選 Anvil/LLM 整合測試 |
| web3-ide（Spring Boot 3.5 / Cloud 2025.0 + Vue） | `web3-ide/` | 可建置；agent-service 可啟動，其他服務為骨架 | agent-service 冒煙測試 + 密碼 fail-fast 檢查 |

## 1. 里程碑

### v0.1 基礎模組（Phase 1–2 收尾）
目標：引擎核心沒有「回傳 Unknown / LOG_ERROR TODO」的假實作；可被外部專案以 `cmake --install` 使用。

| Issue | 主題 | 備註 |
|---|---|---|
| #3 | PluginManager 外掛系統 | 解鎖 Phase 11 Modding 的基礎 |
| #4 | `SystemScheduler::UnregisterSystem` | — |
| #5 | 多尺度時間 + OS 版本偵測 | 時間系統須具決定性（為 #14 lockstep 鋪路） |
| #6 | game 模組 | 把 `src/main.cpp` 的 demo 流程收斂為 Game/Scene 入口 |
| #7 | data 模組（資產、設定、存檔） | 將 `world_save.bin` 格式版本化 |
| #8 | API 綁定（C++/Lua/Python/Rust） | **建議 v0.1 只做 C++ 公開 API + 一種腳本語言**，其餘延後 |
| #9 | 清理殘留 stub | — |

**完成條件**：上述 issue 關閉；CI 全綠；覆蓋率不低於 75%。

### v0.2 可視化與模擬（Phase 3–6）
| Issue | 主題 | 備註 |
|---|---|---|
| #10 | rendering | 優先共用既有 Vulkan device（`src/gpu/VulkanBackend.cpp`） |
| #11 | physics | 固定步長、決定性 |
| #12 | agent 模組與各 agent 的定位 | 見下方第 2 節 |

### v0.3 文明、網路、工具（Phase 7–10）
| Issue | 主題 |
|---|---|
| #13 | civilization（含 SystemDynamics 移植） |
| #14 | networking / lockstep |
| #15 | editor（依賴 #10） |

Phase 11–13（Modding、雲端、AAA）維持在長期 `ROADMAP.md`，暫不開 issue。

## 2. 各元件的關係（提案，與 #12 相關；**本 PR 不搬動任何程式碼**）

```
┌──────────────────────── C++（CMake） ────────────────────────┐
│ 引擎核心 src/core, ecs, math, gpu, iqgpu                       │
│   ├─ 遊戲層 src/ai, world, game, civilization, physics …       │
│   ├─ src/agent（stub）──── 遊戲內「代理人」的高階行為？          │
│   └─ src/self_developing_agent ── 開發工具：程式碼分析/生成     │
└──────────────────────────────────────────────────────────────┘
┌──────────────────────── Java（Maven） ────────────────────────┐
│ web3-agent  ── 獨立函式庫 + CLI：錢包、鏈、P2P、LLM            │
│   └─ web3-ide/agent-service 以 Maven 依賴使用                  │
│ web3-ide    ── Spring Cloud 微服務 + Vue 前端                   │
└──────────────────────────────────────────────────────────────┘
```

目前 C++ 與 Java 兩側**沒有任何程式碼或建置相依**，只共用 repo 與 CI。

### 提案
1. **`src/agent` vs `src/ai`**：`src/ai` 已承擔 NPC 感知/決策/行動。建議將 `src/agent` 定位為「NPC 長期目標、記憶、規劃（含可選 LLM）」，或直接併入 `src/ai` 並刪除 `src/agent`。→ 在 #12 決定。
2. **`src/self_developing_agent`**：屬於**開發工具**而非遊戲執行期。建議改為 `tools/` 下的獨立可執行檔，不再連結進主程式 `StonePotatoes`（目前 `CMakeLists.txt` 會連結），並在 `docs/self_developing_agent.md` 註明實驗性質。
3. **web3-agent / web3-ide**：與遊戲引擎目標不同（鏈上代理人 / IDE）。建議擇一：
   - **A. 拆出獨立 repo**（例如 `StonePotatoes-web3`），本 repo 專注引擎；或
   - **B. 保留於本 repo 但明確標為獨立產品線**：`web3/agent`、`web3/ide` 子目錄，各自 CI，路線圖分開。
   - 若未來要讓遊戲內 NPC 使用鏈上功能，建議透過 **網路協定（HTTP/gRPC 呼叫 agent-service）** 整合，而不是 C++ ↔ JVM 直接綁定。
4. **web3-ide 微服務規模**：eureka / config-server / gateway 對目前只有一個業務服務（agent-service）而言偏重。建議凍結於現狀，直到出現第二個業務服務。

### 需要擁有者決定
- [ ] `src/agent` 保留、併入 `src/ai`，或刪除？
- [ ] `self_developing_agent` 是否移出主程式？
- [ ] web3 部分走方案 A（拆 repo）或 B（同 repo 分產品線）？
- [ ] #8 v0.1 要支援哪一種腳本語言？

## 3. 工程慣例（已在 CI 中落實）
- 每個 PR 需通過：C++ build + ctest、C++ coverage、web3-agent `mvn test`、web3-ide `mvn verify` + 密碼 fail-fast、gitleaks
- 新功能附單元測試（`tests/unit/`）；不接受「回傳假值 + TODO」的實作（呼應 `ROADMAP.md`「No Fake Implementations」）
- 秘密一律由環境變數提供，不得有預設值
