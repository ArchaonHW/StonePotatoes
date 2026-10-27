# StonePotatoes 近期路線圖（v0.1 → v0.3）

> 本文件是**可執行的近期計畫**，以 GitHub issues #3–#15 與里程碑為單位追蹤。
> 長期 14 階段願景（Phase 0–13）仍保留於根目錄 [`ROADMAP.md`](../ROADMAP.md)。
> 最後更新：2026-10-19（台北時間）

## 0. 現況快照

| 區塊 | 位置 | 狀態 | 測試／CI |
|---|---|---|---|
| C++ 引擎核心（core / ecs / game / math / iqgpu / gpu / ai / world） | `src/` | 可運作；含 ECS、Game/Scene lifecycle、wave 平行排程、IQGPU CPU 後端、Vulkan 計算後端、NPC 切片、程序化世界串流 | `ctest` 預設註冊 14 項（Linux Vulkan/Xvfb 環境另加 2 項；Vulkan 無 GPU 時 unit 測試 Skipped） |
| C++ 未完成模組 | `src/{rendering,physics,civilization,networking,editor}` | rendering 已有 Vulkan/ImGui 2D、CPU 投影 3D、深度排序與 flat-shaded PBR 切片；editor 的核心 entity 編輯切片已完成，含獨立 ECS hierarchy、建立／選取／刪除／改名、標籤存檔載入與 Inspector 身分資訊；physics 的 Phase 4 核心切片已完成，含固定 tick、旋轉 OBB 接觸、摩擦／恢復係數與平移／旋轉 swept CCD；civilization 已提供可安裝 stock-flow 經濟、單區域 demographic 與 annual age-cohort population model，以及有向外交關係模型、月度軍事補給／損耗與單次同步交戰解析模型；networking 的 Phase 9 lockstep 核心切片已完成並可安裝；完整子系統仍在後續路線圖 | `renderer3d_geometry_unit`、`editor_session_unit`、`editor_runtime_smoke`（Vulkan/Xvfb）、`physics_fixed_tick_unit`、`ported_utils_unit`、`install_consumer` |
| self_developing_agent | `tools/self_developing_agent/` | 實驗性獨立開發工具；不連結至遊戲執行檔 | CMake target `StonePotatoesSelfDevelopingAgent` |
| web3-agent（Java 25） | `web3-agent/` | 可運作；EIP-1559、合約、P2P、LLM | `mvn test`（離線）+ 可選 Anvil/LLM 整合測試 |
| web3-ide（Spring Boot 3.5 / Cloud 2025.0 + Vue） | `web3-ide/` | 可建置；agent-service 可啟動，其他服務為骨架 | agent-service 冒煙測試 + 密碼 fail-fast 檢查 |

已提供可安裝的 headless CMake 套件 targets：`StonePotatoes::Core`、`StonePotatoes::ECS`、`StonePotatoes::Physics`、`StonePotatoes::Civilization`、`StonePotatoes::Networking`、`StonePotatoes::Game`、`StonePotatoes::CppAPI`、`StonePotatoes::Data`、`StonePotatoes::Math`、`StonePotatoes::AI`、`StonePotatoes::World`。C++ 應用可包含 `<api/cpp/StonePotatoes.h>`，使用 `StonePotatoes::API::Runtime` 設定 ECS、掛載場景並控制啟停／更新。`install_consumer` 會在獨立 CMake 專案連結公開 facade，另驗證 PluginManager 依賴載入與反向卸載、場景進出與延遲切換、變動／固定步長更新、Physics2D 固定 tick 力與重力積分／高速撞牆／摩擦、文明經濟存量同步轉移／事件／KPI 探針、lockstep 初始延遲與 peer 指令同步／checksum mismatch、存檔往返與錯誤存檔拒絕且不改變既有世界，以及資產文字／binary 載入、快取失效、舊 handle 存活和根目錄限制；GLFW 視窗層仍只供 repo 內建置使用。Lua/Python/Rust 綁定仍延後至有具體需求。

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
| #10 | rendering | [基礎切片完成] 共用 Vulkan context、swapchain 呈現、ImGui Vulkan 與既有點／圓 2D API 已接入正式 Renderer；新增 RGBA8 atlas texture handle、UV sprite、旋轉／tint、layer sorting 及整數 tick 驅動的 loop/one-shot atlas animation，透過 Dear ImGui 動態貼圖支援 Vulkan/OpenGL fallback，並提供共用 atlas 動畫 demo 驗收；另有透視相機、視錐裁切、深度排序的 3D 三角面，以及單方向光 Cook-Torrance metallic-roughness 平面材質切片。下一階段建議先做 Vulkan depth-tested indexed mesh pass，之後再做 2D atlas sprite instancing；選項、延後項目與授權／相容性限制見 [`rendering-technology-review.md`](rendering-technology-review.md) 與 [GitHub Discussion #81](https://github.com/ArchaonHW/StonePotatoes/discussions/81)。 |
| #11 | physics | [核心切片完成] `Physics2DSystem` 提供固定 tick 半隱式 Euler 線性與角速度／扭矩積分、`Rotation2D` 更新、依 entity ID 排序的空間格候選篩選、最多 16 輪 OBB SAT 接觸解析、靜／動庫侖摩擦、可選 `Restitution2D` 恢復係數，以及固定朝向 swept SAT 與旋轉方盒保守推進 CCD；離散與連續接觸以接觸點和轉動慣量計算角向衝量，旋轉碰撞每對候選最多推進 128 次。另含大型碰撞體 fallback、有限事件預算、候選數與收斂診斷。`StonePotatoes::Physics` 已匯出至安裝套件；獨立 consumer 驗證公開 headers、相依連結、固定 tick 力／扭矩積分、高速軸對齊與旋轉碰撞、每秒 8 圈旋轉掃掠、偏心衝擊角速度、動態對撞、摩擦、恢復係數，以及完全同心初始重疊的確定性分離。Phase 4 核心垂直切片已完成；多點接觸流形、進階初始重疊修復、關節與跨平台 bitwise lockstep 屬後續能力，不阻擋本 issue 關閉。設計見 [`physics-design.md`](physics-design.md) |
| #12 | agent 模組與各 agent 的定位 | 決策見 [`ARCHITECTURE.md`](../ARCHITECTURE.md)「Repository Component Boundaries」及下方第 2 節；目前不建立獨立 `src/agent` 執行期模組 |

### v0.3 文明、網路、工具（Phase 7–10）
| Issue | 主題 |
|---|---|
| #13 | civilization（含 SystemDynamics 移植） | [進行中；經濟、人口、文化、外交與軍事模型增量] `StonePotatoes::Civilization` 匯出 stock-flow `SDModel`、區域庫存生產／消耗／貿易、由發貨區資金限制的運輸成本與按買方餘額比例縮減並結算賣方收入的路線交易價格模型、依庫存相對目標量調整且有價格上下限的月度市場價格模型、單區 logistic 與出生／死亡月模型、年度 age-cohort 模型、多區人口遷徙模型、結合年齡世代與區域容量的年度遷徙模型，以及可配置特徵與接觸率的區域文化擴散模型、有向外交關係模型（見 [`diplomacy-model.md`](diplomacy-model.md)）、月度財政預算模型（外部稅基／稅率計稅，支出受國庫餘額限制；模型文件：[`fiscal-budget-model.md`](fiscal-budget-model.md)）、月度部隊／補給存量與補給不足損耗模型，以及單次同步齊射交戰解析（見 [`military-logistics-model.md`](military-logistics-model.md) 與 [`battle-resolution-model.md`](battle-resolution-model.md)）；安裝後 consumer 驗證經濟庫存與資金流、財政預算收入與國庫支出上限、補給短缺導致的部隊損耗、外交關係變化與交戰傷亡上限、價格對短缺／過剩反應、文化同步擴散，以及跨區年齡層遷徙的守恆與容量限制。政治制度、外交行動／條約、多回合戰鬥與兵種／地形規則、文化形成、一般貨幣發行／跨品項結算、人口模型依普查資料校準等仍為後續工作；模型使用方式見 [`population-models.md`](population-models.md)、[`trade-network-model.md`](trade-network-model.md)、[`market-price-model.md`](market-price-model.md) 與 [`culture-diffusion-model.md`](culture-diffusion-model.md)，SystemDynamics 移植來源已列於 `THIRD_PARTY_NOTICES.md` |
| #14 | networking / lockstep | [核心切片完成] `StonePotatoes::Networking` 匯出移植的 `LockstepSync`；安裝後 consumer 驗證 input-delay 封包、tick readiness、peer command 與 checksum mismatch。網路傳輸、可靠重送與斷線復原為後續能力，不阻擋本 issue 關閉；來源已列於 `THIRD_PARTY_NOTICES.md` |
| #15 | editor（#10 前置條件已完成） | [核心切片完成] 獨立的開發用 `StonePotatoesEditor` 視窗程式，不併入遊戲執行檔或安裝套件；以 Renderer/ImGui 顯示 ECS entity hierarchy，支援建立、選取、刪除與重新命名 entity，Inspector 顯示 entity index/generation。ECS 二進位場景存檔／啟動載入使用 `--save FILE`、`--scene FILE`；`--frames N` 提供 Xvfb 啟動／關閉 smoke 驗收，`editor_session_unit` 驗證狀態與存檔往返。通用 component reflection、資產瀏覽器與更完整編輯功能另列後續工作，不阻擋本 issue 關閉。 |

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
