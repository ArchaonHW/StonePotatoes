# IQGPU — Intelligent Quantum GPU Development System
# IQGPU — 智能量子 GPU 開發系統

## Overview | 概述

IQGPU is StonePotatoes' unified compute abstraction: one submission API
routes work to whatever execution backend exists. The design reserves
slots for CPU, GPU, NPU and future accelerators, while honestly
reporting which backend actually ran the work.
IQGPU 是 StonePotatoes 的統一計算抽象：同一提交 API
將工作路由至任何存在的執行後端。設計上為 CPU、GPU、NPU
與未來加速器保留槽位，並誠實回報實際執行工作的後端。

```
Application
     │
     ▼
IQGPURuntime                     (implemented | 已實作)
     │
     ├── ComputeGraph            (implemented | 已實作)
     │     ├── KernelDispatch node
     │     ├── HostTask node
     │     └── Barrier node
     │
     ├── ComputeBuffer<T>        (implemented | 已實作)
     │
     └── Backend
           ├── CPUReference      (implemented | 已實作 — std::thread pool)
           ├── DiscreteGPU       (implemented | 已實作 — Vulkan)
           ├── IntegratedGPU     (planned    | 計畫中)
           ├── NPU               (research   | 研究)
           └── QPU               (research   | 研究 — 無量子硬體)
```

## Implementation Status | 實作狀態

| Component 元件 | Status 狀態 | Evidence 驗證 |
|---|---|---|
| `GPU::ComputeDevice` / `EnumerateDevices` | ✅ Implemented | Enumerates the CPU reference device with real `hardware_concurrency`. 以真實 `hardware_concurrency` 列舉 CPU 參考裝置。 |
| `GPU::ComputeBuffer` / `TypedBuffer<T>` | ✅ Implemented | Type-erased host buffer with `Upload`/`Download` transfer hooks matching GPU semantics. 具 GPU 語意 `Upload`/`Download` 介面的型別抹除主機緩衝。 |
| `GPU::Kernel` / `DispatchParams` | ✅ Implemented | HLSL-style dispatch model (global thread index, group size hint). HLSL 式 dispatch 模型（全域執行緒索引、群組大小提示）。 |
| `GPU::CPUBackend` | ✅ Implemented | Real multithreaded dispatch — contiguous partition (`Dispatch`), atomic grain-stealing (`ParallelFor`), and async submission (`DispatchAsync` → `std::future`). 真實多執行緒派發 — 連續切分（`Dispatch`）、原子粒度竊取（`ParallelFor`）與非同步提交（`DispatchAsync` → `std::future`）。 |
| `GPU::VulkanBackend` | ✅ Implemented | Real Vulkan compute — instance/device/queue init, host-visible coherent buffers, SPIR-V pipelines (GLSL via glslc at build), fence-synced dispatch with GPU timestamp queries. Verified on RTX 5050: u32add bit-identical to CPU, saxpy 4M floats gpu=1.9ms/26.4GB/s err=0. Scope: single compute queue, sync dispatch, host-visible memory only. 真實 Vulkan 計算 — 裝置初始化、host-visible 緩衝、SPIR-V 管線、fence 同步派發 + GPU 時間戳。RTX 5050 實測：u32 與 CPU 位元一致、saxpy 誤差 0。範圍：單佇列、同步派發、僅 host-visible 記憶體。 |
| `GPU::ComputeGraph` | ✅ Implemented | DAG of KernelDispatch/HostTask/Barrier nodes; topological execution; cycle detection; `ExecuteParallel` runs independent nodes concurrently. KernelDispatch/HostTask/Barrier 節點 DAG；拓撲執行；循環偵測；`ExecuteParallel` 並行執行無相依節點。 |
| `IQGPU::KernelOptimizer` | ✅ Implemented | Measurement-driven variant selection: warmup + repeats, median/best/worst, speedup report; results feed the agent's `LearningSystem`. 量測驅動變體選擇：暖身 + 重複量測、中位/最佳/最差、加速比報告；結果回饋 Agent 的 `LearningSystem`。 |
| `IQGPU::ZetaCompute` | ✅ Implemented | Parallel HardyZ zero scan (`FindZeros` → `ZeroSpectrum`), parallel `EvaluateZ`, Gram/Rosser `VerifyCriticalLine`, and `RunAudit` — an 11-check RH finite-verification suite (on-line count, argument-principle winding, Gram blocks, GUE KS, Montgomery R₂, explicit formula, Mertens, Robin, Li λ₁, de Bruijn–Newman). GPU batch RS main-sum via `zeta.comp` (dd-phase GLSL kernel) for t > 5e5; CPU fallback on no f64 or dispatch failure. 平行 HardyZ 零點掃描、平行求值、Gram/Rosser 臨界線驗證與 11 項 RH 有限驗證稽核。t > 5e5 時經 `zeta.comp`（dd 相位 GLSL kernel）GPU 批次 RS 主和；無 f64 或派發失敗時退回 CPU。 |
|
| `Math::Zeta` (`src/math`) | ✅ Implemented | Euler–Maclaurin/Hurwitz ζ, Lanczos Γ, Riemann–Siegel with C0 remainder, Gram points, η/β/ξ, de Bruijn–Newman kernel, argument principle, Chebyshev ψ, Möbius sieve, Mertens, Robin, Li coefficients. EM/Hurwitz ζ、Lanczos Γ、含 C0 餘項的 Riemann–Siegel、Gram 點、η/β/ξ、de Bruijn–Newman 核、輻角原理、Chebyshev ψ、Möbius 篩、Mertens、Robin、Li 係數。 |
| `Math::Zeta::ZeroSpectrum` | ✅ Implemented | Zero table + unfolded spacing, GUE/Poisson KS distances, Montgomery pair correlation, min-spacing (Lehmer-pair) detector. 零點表 + 展開間距、GUE/Poisson KS 距離、Montgomery 配對關聯、最小間距（Lehmer 近重合）偵測。 |
| `Math::Zeta::SpectralPointSampler` | ✅ Implemented | Level-repulsion point placement for world-gen (1D/2D, deterministic per seed). 供世界生成用的層級排斥點分佈（一維/二維、按 seed 確定）。 |
| `Math::Zeta::SpectralNoise` | ✅ Implemented | Deterministic aperiodic noise; frequencies are zero ordinates γ_k. 確定性非週期雜訊；頻率為零點縱座標 γ_k。 |
| `IQGPU::IQGPURuntime` | ✅ Implemented | Runtime facade: device select, buffer factory, dispatch, graph execution. 執行期外觀：裝置選擇、緩衝工廠、派發、圖執行。 |
| `IQGPU::SimulatedAnnealer` | ✅ Implemented | Classical annealing baseline. 傳統退火基準。 |
| `IQGPU::QuantumInspiredAnnealer` | ✅ Implemented | Annealing + long-range "tunneling" jumps (transverse-field analog). 退火 + 長程「穿隧」跳躍（橫向場類比）。 |
| `IQGPU::AmplitudeAmplifier` | ✅ Implemented | Grover-inspired: real amplitude vector, oracle phase-flip + inversion-about-mean, optimal round bound `⌊π/4·√(N/M)⌋`. Grover 啟發：真實振幅向量、神諭相位翻轉 + 均值反轉、最佳輪數上界 `⌊π/4·√(N/M)⌋`。 |
| `IQGPU::ProbabilisticStateSpace` | ✅ Implemented | Amplitude vectors, Born-rule measurement, additive interference. 振幅向量、玻恩定則測量、加法干涉。 |
| D3D12 GPU backend | 📋 Planned | Vulkan backend exists; D3D12 port not started. Vulkan 後端已實作；D3D12 未開始。 |
| Device-local VRAM + staging | 📋 Planned | Buffers are host-visible coherent; discrete-VRAM staging + transfer queue is future work. 緩衝為 host-visible；獨立 VRAM staging 與傳輸佇列為未來工作。 |
| Async/multi-queue scheduling | 📋 Planned | CPU graph runs parallel waves; GPU dispatch is single-queue sync. CPU 圖已波次平行；GPU 派發為單佇列同步。 |
| Real quantum hardware (QPU) | 🔬 Research | **No quantum hardware is claimed.** Quantum-inspired algorithms run classically. **不宣稱量子硬體。** 量子啟發演算法以傳統方式執行。 |

## Measured Results | 實測結果

On the development machine (16 HW threads, MSVC Release):
在開發機器上（16 硬體執行緒、MSVC Release）：

| Demo 演示 | Result 結果 |
|---|---|
| `saxpy` 1,000,000 elements | 8.9 ms across 16 workers — **correct** result verified. 16 worker 耗時 8.9 ms — 結果驗證正確。 |
| ComputeGraph `A→B→C` | Executed in dependency order, correct output. 依相依順序執行，輸出正確。 |
| Rastrigin 8-dim minimization, 20k iters | Classical SA: E=1.00; Quantum-inspired (tunneling): **E=0.00** — reached global minimum. 傳統 SA：E=1.00；量子啟發（穿隧）：**E=0.00** — 達到全域最小值。 |
| AmplitudeAmp, 10 marked in 1024 | 7 Grover rounds → **99.3%** probability mass on marked states (uniform = 1.0%). 7 輪 Grover → 標記狀態機率質量 **99.3%**（均勻 = 1.0%）。 |
| Born-rule measurement, 4k samples | 12.5% / 53.3% / 3.2% / 31.0% vs theoretical 13.3% / 53.3% / 3.3% / 30.0%. 與理論值 13.3/53.3/3.3/30.0% 相符。 |
| KernelOptimizer, uneven 2M workload | `grain_128` grain-stealing beat contiguous partition **1.51×** (11.07 vs 16.67 ms); best variant recorded into agent learning memory. `grain_128` 粒度竊取比連續切分快 **1.51×**（11.07 vs 16.67 ms）；最佳變體已寫入 Agent 學習記憶。 |
| ZetaCompute parallel scan, zeros to t≈2000 | ~100k HardyZ samples across 16 workers; audit completed in ~111 ms. 16 worker 取樣約十萬次 HardyZ；稽核約 111 ms 完成。 |
| RH finite-verification audit, T=60 | **11/11 PASS** — winding count 13 equals on-line zeros 13 (no off-line zeros in the strip); Gram/Rosser 41/41; GUE KS 0.045 vs Poisson 0.320; explicit-formula max err 0.255; Li λ₁ within tail bound; de Bruijn–Newman H_λ zeros real. **11/11 通過** — 環繞數 13 等於線上零點 13（帶內無離線零點）；Gram/Rosser 41/41；GUE KS 0.045 對 Poisson 0.320；顯式公式最大誤差 0.255；Li λ₁ 在尾項界內；de Bruijn–Newman H_λ 零點皆實。 |
|  GPU batch Z(t) | 5 Gram points (t≈5k–423k): error ≤ 3.5×Gabcke bound (Taylor cos remainder); RTX 5050 f64 kernel loads logK table once; GPU dispatches for  at t>5e5 (gpu=1 flag set). 5 個 Gram 點（t≈5k–423k）：誤差 ≤ 3.5×Gabcke 界（Taylor cos 餘項）；RTX 5050 f64 kernel 一次載入 logK 表；t>5e5 的  啟用 GPU 派發（gpu=1）。 |

## Quantum-Inspired ≠ Quantum | 量子啟發 ≠ 量子

All algorithms in this module run on classical hardware with classical
complexity. `AmplitudeAmplifier`, for instance, evaluates its oracle
over the full domain (O(N)) — the "speedup" it demonstrates is the
correct amplitude mathematics of Grover iteration simulated in
software, not a quantum speedup.
本模組所有演算法皆以傳統複雜度在傳統硬體上執行。
例如 `AmplitudeAmplifier` 對整個定義域評估其神諭（O(N)）—
其展示的「加速」是以軟體模擬 Grover 迭代的正確振幅數學，
而非量子加速。

## Usage | 用法

```cpp
#include "iqgpu/IQGPURuntime.h"
#include "iqgpu/QuantumInspired.h"

using namespace StonePotatoes;

// Runtime | 執行期
IQGPU::IQGPURuntime iqgpu;
iqgpu.Initialize();                      // selects best device (CPU today)

// Parallel kernel | 平行核心
auto* buf = iqgpu.CreateTypedBuffer<float>(N);
GPU::Kernel k("name", [](uint32_t gid, void* ud) { /* per-thread work */ });
GPU::DispatchParams dp{ N, 64, &userData };
auto stats = iqgpu.Dispatch(k, dp);      // {dispatchMs, threadsUsed, itemsProcessed}

// Compute graph | 計算圖
auto& g = iqgpu.GetGraph();
auto a = GPU::ComputeNode::MakeKernel("A", k, N, ud);
auto b = GPU::ComputeNode::MakeTask("B", []{ /* host work */ });
b->AddDependency(a);
g.AddNode(a); g.AddNode(b);
auto result = iqgpu.ExecuteGraph();      // topological order, cycle-safe

// Quantum-inspired | 量子啟發
IQGPU::AmplitudeAmplifier aa;
auto r = aa.Search(1024, [](uint32_t i){ return i % 97 == 0 && i != 0; });
// r.probabilities — marked states amplified to ~99%
```

## ZetaCompute · ζ 計算

`IQGPU::ZetaCompute` integrates `Math::Zeta` with the runtime: parallel
`FindZeros` scans (HardyZ sign-change + bisection into a `ZeroSpectrum`),
parallel `EvaluateZ`, Gram/Rosser `VerifyCriticalLine`, and `RunAudit` — an
11-check RH finite-verification suite: on-line count, argument-principle
winding, Gram blocks, GUE KS, Montgomery R₂, explicit formula, Mertens,
Robin, Li λ₁, de Bruijn–Newman.
`IQGPU::ZetaCompute` 將 `Math::Zeta` 整合進執行期：平行 `FindZeros`
掃描（HardyZ 變號 + 二分法產生 `ZeroSpectrum`）、平行 `EvaluateZ`、
Gram/Rosser `VerifyCriticalLine`，以及 `RunAudit` — 11 項 RH 有限驗證。

```
[Zeta] Found 13 zeros in [13,60): first=14.134725 (true 14.134725)
[RH] zeros.on_line      13 (N0(T)=12.91)                    PASS
[RH] winding.strip_count 13                                 PASS
[RH] gram.critical_line 41/41 bad_gram=0                    PASS
[RH] gue.ks             0.0453 (poisson 0.3204)             PASS
[RH] explicit.psi_max_err 0.2546                            PASS
[RH] ... Audit: 11/11 checks passed — falsifies, never proves
```

Parallel scan to t≈2000 samples ~100k HardyZ points on 16 workers in ~111 ms.
平行掃描至 t≈2000 在 16 worker 上取樣約十萬點，約 111 ms。

## What RH Finite Verification Means · RH 有限驗證的意義

- **Every check can falsify, never prove.** The winding-number count of ξ
  along the strip boundary agreeing with the on-line sign-change count
  certifies *no off-line zeros below T* — the strongest statement numerical
  analysis supports. **每項檢驗只能否證、不能證明。** ξ 沿帶邊界的環繞數
  與線上變號次數一致，可確認 *T 以下無離線零點* — 數值分析的最強結論。
- `SpectralNoise` / `SpectralPointSampler` turn the zero spectrum into
  world-gen primitives: level repulsion gives non-clumping, non-lattice
  spawn distributions. `SpectralNoise` / `SpectralPointSampler` 將零點頻譜
  化為世界生成原語：層級排斥產生不聚集、非格狀的分佈。

## Turing-Certified Verification · Turing 認證驗證

`VerifyCriticalLine(-1, n)` upgrades the Gram/Rosser count to a Turing-method
certificate (Brent 1979, Thm 3.2): if the last K ≥ 0.0061 ln²g + 0.08 ln g
Gram blocks satisfy Rosser's rule then N(g_n) ≤ n+1; together with the n+1
sign changes found on the line from g_{−1}, **N(g_n) = n+1 exactly — every
zero with 0 < γ < g_n is on the critical line.** Each counted sample must
clear the HardyZ error bound (Gabcke: 0.053·t^{−5/4} after C0+C1; measured
6.7e-5 at t≈200 vs bound 7.0e-5); samples inside the band are nudged, and
any survivor (`uncertainSigns`) blocks certification.
`VerifyCriticalLine(-1, n)` 將 Gram/Rosser 計數升級為 Turing 法認證：
最後 K 個 Gram 區塊滿足 Rosser 規則 ⇒ N(g_n) ≤ n+1；配合線上找到的 n+1 個
變號 ⇒ **N(g_n)=n+1 精確成立：0<γ<g_n 的零點全在臨界線上。**
每個計數樣本須超過 HardyZ 誤差界（Gabcke），否則不予認證。

CLI `StonePotatoesRHVerify [gramLast] [samples] [--selftest] [--audit]`
(key=value output, exit 0 only on PASS; ctest `rh_verify_cli`):

| Range | Result (16 workers) |
|---|---|
| g ≤ 20,000 (t ≤ 18,048) | 20,001 zeros, 1,838 Gram failures resolved, certified N=19,999, 24 ms |
| g ≤ 10⁶ (t ≤ 600,270) | 1,000,001 zeros, 130,102 Gram failures, 114,766 Rosser blocks, 0 uncertain signs, certified N(600268.8)=999,998, 3.6 s |
| g ≤ 10⁷ (t ≤ 4,992,382) | 10,000,001 zeros, 1,463,568 Gram failures, 1,238,624 Rosser blocks, 10,176 refined intervals, 0 uncertain signs (min margin 1.04× bound), certified N(4992379.49)=9,999,996, 109 s |

Caveat: IEEE double without interval arithmetic — strong numerical evidence,
not a machine-checked proof. Records (Platt–Trudgian 2021) reach 3·10¹².
注意：使用 IEEE double、未採區間算術——屬強數值證據而非機器驗證證明；
現有紀錄（Platt–Trudgian 2021）已達 3·10¹²。

## Engine Applications · 引擎應用

- `IQGPU::ZetaCompute::SelfTest()` — golden known-answer workload for any
  IQGPU backend (N(1000)=649, Odlyzko zeros, Gram failure at 126, Turing
  certificate, bit-identical results 1 vs N workers for lockstep/replay).
  任何 IQGPU 後端的黃金已知答案自檢（含 1 vs N worker 位元一致性）。
- `Math::Zeta::SpectralScatter` — chunk-seamless 2D scatter. Spectral
  rigidity (|x_k − k| = |S(γ_k)| small) makes every point addressable by
  (row, index), so a big query and any chunk tiling return the same set.
  跨 chunk 無接縫散佈：頻譜剛性使每點可由（列、索引）直接定址。
- `World::ResourceScatter` (`src/world/procedural`) — streaming resource
  layers (spacing, density, SpectralNoise biome mask) with stable 64-bit
  node ids for saves / networking / harvested bitsets.
  串流資源圖層（間距、密度、頻譜雜訊生態遮罩），節點 id 跨工作階段穩定。

## Roadmap | 路線圖

1. **Phase 2a (done)**: CPU reference backend + graph + QI algorithms.
   已完成：CPU 參考後端 + 計算圖 + 量子啟發演算法。
2. **Phase 2b (done)**: Vulkan compute backend — real device init,
   SPIR-V pipelines (glslc), host-visible buffers, fence-synced dispatch
   + GPU timestamps; bit-exact u32 verified vs CPU on RTX 5050.
   已完成：Vulkan 計算後端 — 真實裝置初始化、SPIR-V 管線（glslc）、
   host-visible 緩衝、fence 同步派發 + GPU 時間戳；
   RTX 5050 上 u32 與 CPU 位元一致。
3. **Phase 2c**: Device-local VRAM + staging, async dispatch,
   multi-queue graph execution, barriers with real memory ordering.
   獨立 VRAM + staging、非同步派發、多佇列圖執行、真實記憶體序屏障。
4. **Phase 8**: GPU-resident simulation kernels driven by the same graph.
   以同一圖驅動 GPU 常駐模擬核心。
