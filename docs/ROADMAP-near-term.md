# StonePotatoes 近期路線圖（v0.1 → v0.3）

> 本文件是**可執行的近期計畫**，以 GitHub issues #3–#15 與里程碑為單位追蹤。
> 長期 14 階段願景（Phase 0–13）仍保留於根目錄 [`ROADMAP.md`](../ROADMAP.md)。
> 最後更新：2026-10-14（台北時間）

## 0. 現況快照

| 區塊 | 位置 | 狀態 | 測試／CI |
|---|---|---|---|
| C++ 引擎核心（core / ecs / game / math / iqgpu / gpu / ai / world） | `src/` | 可運作；含 ECS、Game/Scene lifecycle、wave 平行排程、IQGPU CPU 後端、Vulkan 計算後端、NPC 切片、程序化世界串流 | `ctest` 註冊 11 項（Vulkan 無 GPU 時 Skipped；執行結果以 CI 為準） |
| C++ 未完成模組 | `src/{data,rendering,physics,agent,civilization,networking,editor,api/*}` | 多數仍為 stub；civilization/networking 含已移植工具，但完整子系統尚未完成 | — |
| self_developing_agent | `tools/self_developing_agent/` | 實驗性獨立開發工具；不連結至遊戲執行檔 | CMake target `StonePotatoesSelfDevelopingAgent` |
| web3-agent（Java 25） | `web3-agent/` | 可運作；EIP-1559、合約、P2P、LLM | `mvn test`（離線）+ 可選 Anvil/LLM 整合測試 |
| web3-ide（Spring Boot 3.5 / Cloud 2025.0 + Vue） | `web3-ide/` | 可建置；agent-service 可啟動，其他服務為骨架 | agent-service 冒煙測試 + 密碼 fail-fast 檢查 |

已提供可安裝的 headless CMake 套件 targets：`StonePotatoes::Core`、`StonePotatoes::ECS`、`StonePotatoes::Game`、`StonePotatoes::Math`、`StonePotatoes::AI`、`StonePotatoes::World`。`install_consumer` 會在獨立 CMake 專案連結 ECS 與 Game，驗證場景生命週期、世界更新與存檔往返；GLFW 視窗層仍只供 repo 內建置使用。

遊戲主程式建置輸出固定在 `build/bin/<Config>/StonePotatoes[.exe]`（Visual Studio 多組態）或 `build/bin/StonePotatoes`（單組態）；`cmake --install` 會將遊戲、headless C++ 開發套件與文件整理到安裝前綴，`cpack` 會在 `build/packages/` 產生 ZIP（Windows）或 TGZ（Linux/macOS）。`self_developing_agent` 是開發工具，不會打進遊戲套件。

## 1. 里程碑

### v0.1 基礎模組（Phase 1–2 收尾）
目標：引擎核心沒有「回傳 Unknown / LOG_ERROR TODO」的假實作；可被外部專案以 `cmake --install` 使用。

| Issue | 主題 | 備註 |
|---|---|---|
| #3 | PluginManager 外掛系統 | 解鎖 Phase 11 Modding 的基礎 |
| #4 | `SystemScheduler::UnregisterSystem` | — |
| #5 | 多尺度時間 + OS 版本偵測 | 時間系統須具決定性（為 #14 lockstep 鋪路） |
| #6 | game 模組 | 已有 headless Game/Scene lifecycle、逐幀邊界切換與設定驅動視窗；demo 主迴圈由使用者關窗結束。固定步長與場景資源管理仍待補 |
| #7 | data 模組（資產、設定、存檔） | `world_save.bin` v1 已文件化並拒絕損壞／不支援版本；資產載入與設定管理仍待實作 |
| #8 | API 綁定（C++/Lua/Python/Rust） | **建議 v0.1 先做 C++ 公開 API；腳本綁定待具體需求出現後再選一種** |
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

## 2. 各元件的關係（#12 邊界建議）

```
┌──────────────────────── C++（CMake） ────────────────────────┐
│ 引擎核心 src/core, ecs, math, gpu, iqgpu                       │
│   ├─ 遊戲層 src/ai, world, game, civilization, physics …       │
│   ├─ src/agent（stub）──── 遊戲內「代理人」的高階行為？          │
└──────────────────────────────────────────────────────────────┘
┌──────────────────────── 開發工具（CMake）─────────────────────┐
│ tools/self_developing_agent ── 程式碼分析/生成，不屬於遊戲執行期 │
└──────────────────────────────────────────────────────────────┘
┌──────────────────────── Java（Maven） ────────────────────────┐
│ web3-agent  ── 獨立函式庫 + CLI：錢包、鏈、P2P、LLM            │
│   └─ web3-ide/agent-service 以 Maven 依賴使用                  │
│ web3-ide    ── Spring Cloud 微服務 + Vue 前端                   │
└──────────────────────────────────────────────────────────────┘
```

目前 C++ 與 Java 兩側**沒有任何程式碼或建置相依**，只共用 repo 與 CI。

### 建議採用的決策
1. **`src/agent` vs `src/ai`**：保留 `src/ai` 作為遊戲內 NPC 行為模組，移除目前空的 `src/agent` stub target；未來出現獨立且可測試的長期目標／記憶／規劃 API 時，再以 `src/ai/agent` 或明確的子模組加入。
2. **`tools/self_developing_agent`**：已移出 `StonePotatoes` 遊戲執行檔，提供獨立命令列入口；以 `StonePotatoesSelfDevelopingAgent <development goal>` 執行。它目前會在工作目錄建立 `agent_workspace/`，仍屬實驗工具，不會自動修改引擎原始碼。
3. **web3-agent / web3-ide**：目前留在同一 repo 作為分離產品線，各自維持 Maven/前端建置與 CI。現在沒有 C++ 相依，拆 repo 會增加版本與維護成本；只有在獨立發布、權限或維護者明確分流時再拆。遊戲整合維持 HTTP/gRPC 邊界。
4. **web3-ide 微服務規模**：凍結現有服務骨架，等第二個業務服務出現再擴充。
5. **#8 腳本 API**：v0.1 先提供穩定 C++ API，不承諾 Lua/Python/Rust 綁定；待遊戲切片確認腳本需求後，再選一種語言。

## 3. 工程慣例（已在 CI 中落實）
- 每個 PR 需通過：C++ build + ctest、C++ coverage、web3-agent `mvn test`、web3-ide `mvn verify` + 密碼 fail-fast、gitleaks
- 新功能附單元測試（`tests/unit/`）；不接受「回傳假值 + TODO」的實作（呼應 `ROADMAP.md`「No Fake Implementations」）
- 秘密一律由環境變數提供，不得有預設值
