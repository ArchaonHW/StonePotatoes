# StonePotatoes 近期路線圖（v0.1 → v0.3）

> 本文件是**可執行的近期計畫**，以 GitHub issues #3–#15 與里程碑為單位追蹤。
> 長期 14 階段願景（Phase 0–13）仍保留於根目錄 [`ROADMAP.md`](../ROADMAP.md)。
> 最後更新：2026-10-15（台北時間）

## 0. 現況快照

| 區塊 | 位置 | 狀態 | 測試／CI |
|---|---|---|---|
| C++ 引擎核心（core / ecs / game / math / iqgpu / gpu / ai / world） | `src/` | 可運作；含 ECS、Game/Scene lifecycle、wave 平行排程、IQGPU CPU 後端、Vulkan 計算後端、NPC 切片、程序化世界串流 | `ctest` 預設註冊 12 項（Linux Vulkan/Xvfb 環境另加 1 項；Vulkan 無 GPU 時 unit 測試 Skipped） |
| C++ 未完成模組 | `src/{rendering,physics,civilization,networking,editor}` | rendering 已有 Vulkan/ImGui 2D 路徑；physics 有固定 tick、確定順序 AABB 接觸與空間格 broad phase 首切片；civilization/networking 含已移植工具，其餘完整子系統尚未完成 | `physics_fixed_tick_unit`；Vulkan runtime 見 #10 |
| self_developing_agent | `tools/self_developing_agent/` | 實驗性獨立開發工具；不連結至遊戲執行檔 | CMake target `StonePotatoesSelfDevelopingAgent` |
| web3-agent（Java 25） | `web3-agent/` | 可運作；EIP-1559、合約、P2P、LLM | `mvn test`（離線）+ 可選 Anvil/LLM 整合測試 |
| web3-ide（Spring Boot 3.5 / Cloud 2025.0 + Vue） | `web3-ide/` | 可建置；agent-service 可啟動，其他服務為骨架 | agent-service 冒煙測試 + 密碼 fail-fast 檢查 |

已提供可安裝的 headless CMake 套件 targets：`StonePotatoes::Core`、`StonePotatoes::ECS`、`StonePotatoes::Game`、`StonePotatoes::CppAPI`、`StonePotatoes::Data`、`StonePotatoes::Math`、`StonePotatoes::AI`、`StonePotatoes::World`。C++ 應用可包含 `<api/cpp/StonePotatoes.h>`，使用 `StonePotatoes::API::Runtime` 設定 ECS、掛載場景並控制啟停／更新。`install_consumer` 會在獨立 CMake 專案連結公開 facade，另驗證 PluginManager 依賴載入與反向卸載、場景進出與延遲切換、變動／固定步長更新、存檔往返與錯誤存檔拒絕且不改變既有世界，以及資產文字／binary 載入、快取失效、舊 handle 存活和根目錄限制；GLFW 視窗層仍只供 repo 內建置使用。Lua/Python/Rust 綁定仍延後至有具體需求。

遊戲主程式建置輸出固定在 `build/bin/<Config>/StonePotatoes[.exe]`（Visual Studio 多組態）或 `build/bin/StonePotatoes`（單組態）；`cmake --install` 會將遊戲、headless C++ 開發套件與文件整理到安裝前綴，`cpack` 會在 `build/packages/` 產生 ZIP（Windows）或 TGZ（Linux/macOS）。`self_developing_agent` 是開發工具，不會打進遊戲套件。

## 1. 里程碑

### v0.1 基礎模組（Phase 1–2 收尾）
目標：引擎核心沒有「回傳 Unknown / LOG_ERROR TODO」的假實作；可被外部專案以 `cmake --install` 使用。

| Issue | 主題 | 備註 |
|---|---|---|
| #3 | PluginManager 外掛系統 | 已由 PR #18 完成並關閉；具依賴排序、生命週期、動態載入與單元測試；安裝後 public API 驗收補充中 |
| #4 | `SystemScheduler::UnregisterSystem` | 已支援更新期間延遲移除，並呼叫可選的 `Shutdown(world)` |
| #5 | 多尺度時間 + OS 版本偵測 | 已完成：明確 delta 決定性推進、多尺度累積時間與原生 OS 版本識別 |
| #6 | game 模組 | 核心範圍已完成：headless Game/Scene lifecycle、逐幀邊界切換、可選固定步長及補步上限、設定驅動視窗與正常主迴圈。場景資源生命週期屬後續增量，不阻擋本 issue 完成 |
| #7 | data 模組（資產、設定、存檔） | 核心範圍已完成：存檔 v1 格式及錯誤輸入拒絕、`AssetCache` 根目錄限制與 binary/text 載入和快取失效。既有 `ConfigManager` 保留在 `core`，不因本 issue 搬移模組 |
| #8 | API 綁定（C++/Lua/Python/Rust） | C++ headless facade 與安裝後 consumer 驗收已實作；Lua/Python/Rust bindings 延後，未建立假 target |
| #9 | 清理殘留 stub | 移除沒有實作與消費者的 `src/agent`、Python/Lua API targets；NPC 行為由 `src/ai` 負責，未來需要獨立 agent API 時再新增明確子模組 |

**完成條件**：上述 issue 關閉；CI 全綠；覆蓋率不低於 75%。

### v0.2 可視化與模擬（Phase 3–6）
| Issue | 主題 | 備註 |
|---|---|---|
| #10 | rendering | [進行中] 共用 Vulkan context、swapchain 呈現、ImGui Vulkan 與既有點／圓 2D API 已接入正式 Renderer；RTX 5050 上已本機驗證 renderer 初始化、逐幀執行與正常關閉，CPU-only 建置保留 OpenGL fallback。Linux Vulkan CI 已設定透過 Xvfb 執行 `StonePotatoesVulkanComputeRendererDemo --frames 3`，驗收同一 `VkDevice`、64 筆 SAXPY、三幀呈現與正常關閉；設計見 [`rendering-vulkan-design.md`](rendering-vulkan-design.md) |
| #11 | physics | [進行中] 新增 `Physics2DSystem`：固定 tick 下以半隱式 Euler 積分重力與一次性累積力；補上依 entity ID 排序的空間格候選篩選與最多 16 輪 AABB 接觸解析，含大型碰撞體 fallback、候選數與收斂診斷。旋轉、摩擦、連續碰撞與跨平台 bitwise lockstep 延後；設計見 [`physics-design.md`](physics-design.md) |
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
│   ├─ 不保留空的 src/agent target；NPC 行為由 src/ai 負責       │
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
1. **`src/agent` vs `src/ai`**：保留 `src/ai` 作為遊戲內 NPC 行為模組；空的 `src/agent` stub target 已移除。未來出現獨立且可測試的長期目標／記憶／規劃 API 時，再以 `src/ai/agent` 或明確的子模組加入。
2. **`tools/self_developing_agent`**：已移出 `StonePotatoes` 遊戲執行檔，提供獨立命令列入口；以 `StonePotatoesSelfDevelopingAgent <development goal>` 執行。它目前會在工作目錄建立 `agent_workspace/`，仍屬實驗工具，不會自動修改引擎原始碼。
3. **web3-agent / web3-ide**：目前留在同一 repo 作為分離產品線，各自維持 Maven/前端建置與 CI。現在沒有 C++ 相依，拆 repo 會增加版本與維護成本；只有在獨立發布、權限或維護者明確分流時再拆。遊戲整合維持 HTTP/gRPC 邊界。
4. **web3-ide 微服務規模**：凍結現有服務骨架，等第二個業務服務出現再擴充。
5. **#8 腳本 API**：v0.1 提供穩定 C++ API；Lua/Python/Rust 綁定尚無具體需求，對應的空 CMake placeholders 已移除，待遊戲切片確認腳本需求後再選一種語言。

## 3. 工程慣例（已在 CI 中落實）
- 每個 PR 需通過：C++ build + ctest、C++ coverage、web3-agent `mvn test`、web3-ide `mvn verify` + 密碼 fail-fast、gitleaks
- 新功能附單元測試（`tests/unit/`）；不接受「回傳假值 + TODO」的實作（呼應 `ROADMAP.md`「No Fake Implementations」）
- 秘密一律由環境變數提供，不得有預設值
