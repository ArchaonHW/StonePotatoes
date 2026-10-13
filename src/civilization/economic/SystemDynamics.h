// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
// Derived from PotatoEngine Simulation/SystemDynamics.{h,cpp} (MIT). See THIRD_PARTY_NOTICES.md.
// 衍生自 PotatoEngine Simulation/SystemDynamics.{h,cpp}（MIT），詳見 THIRD_PARTY_NOTICES.md。
#pragma once

#include <functional>
#include <limits>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Economic {

/**
 * SDState — read-only state view during flow/aux evaluation.
 * Lambdas read current (t) stocks, params, and already-evaluated aux values,
 * plus previous-step (t-1) aux (for information delay/smoothing chains).
 * SDState — 流量/輔助變數求值時的唯讀狀態視圖。
 * lambda 透過它讀本步（t）的存量、參數與已求值的輔助變數，
 * 以及上一步（t-1）的輔助變數（資訊延遲/平滑鏈用）。
 */
class SDState {
public:
    float S(const std::string& stock) const;  // stock at t | 存量（t）
    float P(const std::string& param) const;  // parameter | 參數
    float A(const std::string& aux) const;    // aux at t (already computed) | 輔助變數（本步已算）
    float APrev(const std::string& aux) const;// aux at t-1 | 輔助變數（上一步值）
    int Month() const { return m_Month; }

private:
    friend class SDModel;
    const std::map<std::string, float>* m_Stocks = nullptr;
    const std::map<std::string, float>* m_Params = nullptr;
    const std::map<std::string, float>* m_Aux = nullptr;      // current step
    const std::map<std::string, float>* m_AuxPrev = nullptr;  // previous step
    int m_Month = 0;
};

/**
 * SDModel — system dynamics stock-flow simulator (discrete Euler stepping).
 *
 * Modeling API:
 *   - Stock: accumulative quantity with initial value and bounds (CLAMP).
 *   - Flow: net flow into a stock per step (evaluated at t).
 *   - Aux: auxiliary variable (ratio, efficiency, gap — no natural accumulation, recomputed each step).
 *   - Param: exogenous parameter (can be adjusted at runtime for scenario scanning).
 *   - Event: discrete impulse at a specific month (one-time stock delta).
 *   - RandomEvent: probabilistic event at a specific month (random shock layer).
 *
 * Update semantics (Euler): aux evaluated in registration order → all flows evaluated at t →
 *   stock[t+1] = stock[t] + dt·net_flow applied synchronously → CLAMP → record.
 * This guarantees mass conservation during A↔B transfers (flows all see t, not mixed temporal state).
 *
 * Delay modeling: use APrev for first-order exponential smoothing (information delay).
 *   AddAux("perceived", [](s){ return s.APrev("perceived") + (s.S("real") - s.APrev("perceived")) / s.P("tau"); });
 * Multi-stage delay chains chain multiple aux. Pipeline delays use intermediate stocks.
 *
 * Default dt=1 (month model); SetDt can switch to week/quarter. Pure CPU, deterministic, headless-testable.
 *
 * Use cases: RTS economy (funds/materials/capacity/orders feedback loops), population & morale,
 *   supply-line stocks, AI strategic resource simulation.
 * SDModel — 系統動力學存量-流量模擬器（離散步進，Euler 語義）。
 * 建模 API：Stock（存量）、Flow（流量）、Aux（輔助變數）、Param（參數）、Event（離散事件）、RandomEvent（機率事件）。
 * 更新語義（Euler）：aux 依註冊序求值 → 所有 flow 以 t 狀態求值 → 同步套用 stock[t+1] = stock[t] + dt·net_flow → CLAMP → 記錄。
 * 保證 A↔B 互相轉移時總量守恆（flow 都看到 t 而非混合時態）。
 * 延遲建模：以 APrev 做一階指數平滑。多階延遲鏈串多個 aux；實體延遲用中間存量串。
 * 預設 dt=1（月模型）；SetDt 可切週/季。純 CPU、決定性、headless 可測。
 * 用途：RTS 經濟、人口與民心、補給線存量、AI 敵國戰略資源模擬。
 */
class SDModel {
public:
    using Fn = std::function<float(const SDState&)>;

    // ---- Modeling (any order; aux/flow evaluation order = registration order) ----
    // 存量：lo/hi 為 CLAMP 界（預設 0..+inf 防負庫存類錯誤）
    void AddStock(const std::string& name, float initial,
                  float lo = 0.f,
                  float hi = std::numeric_limits<float>::infinity());
    void SetParam(const std::string& name, float value);
    void AddAux(const std::string& name, Fn fn);
    // Net flow into a stock (multiple AddFlow with same stock name = flows sum)
    // 某存量的淨流量（多個 AddFlow 同名存量 = 流量相加）
    void AddFlow(const std::string& stock, Fn fn);
    // Discrete event: at month, apply one-time delta to stock
    // 離散事件：到達 month 步時對存量一次性加 delta
    void AddEvent(int month, const std::string& stock, float delta);
    // Probabilistic event: at month, trigger with probability prob (random shock layer).
    // SetSeed controls seed; same seed = fully reproducible (testable).
    // 機率性事件：到達 month 步時以 prob 機率觸發。SetSeed 控制種子；同 seed 結果完全可重現。
    void AddRandomEvent(int month, const std::string& stock, float delta, float prob);
    void SetSeed(uint32_t seed);

    // Material/information delay chain: diffuse src output over stages first-order delays,
    // total delay time ≈ tau (tau/stages per stage). Generates aux name, implemented via APrev chain.
    // 物質/資訊延遲鏈：把 src 的輸出經 stages 級一階延遲攤開，總延遲時間約 tau。
    void AddDelayChain(const std::string& name, Fn src, int stages, float tau);

    // KPI observation probe: record fn evaluation each step into ProbeHistory.
    // KPI 觀測探針：每步記錄 fn 求值到 ProbeHistory。
    void AddProbe(const std::string& name, Fn fn);

    void SetDt(float dt) { m_Dt = dt; }

    // ---- Execution ----
    void Step();          // advance one dt | 推進一個 dt
    void Run(int steps);  // run N steps | 連跑 N 步

    // ---- Query ----
    float Stock(const std::string& name) const;
    float Aux(const std::string& name) const;   // previous-step evaluation result | 上一步求值結果
    int Month() const { return m_Month; }
    // History trajectory (includes initial value, length = month+1) | 歷史軌跡（含初始值）
    const std::vector<float>& History(const std::string& stock) const;
    // Probe history (one point per step, length = month) | 探針歷史（每步一點）
    const std::vector<float>& ProbeHistory(const std::string& name) const;
    // Breach months: list of months where probe value crosses above threshold
    // (edge-triggered: previous ≤ threshold, current > threshold)
    // 觸發月份掃描：probe 值上穿 threshold 的月份清單（邊沿觸發語義）
    std::vector<int> BreachMonths(const std::string& probe, float threshold) const;

private:
    struct StockDef { float value, lo, hi; };
    struct Event { int month; std::string stock; float delta; float prob; };

    std::map<std::string, StockDef> m_Stocks;
    std::map<std::string, float> m_Params;
    std::vector<std::pair<std::string, Fn>> m_AuxFns;   // ordered
    std::vector<std::pair<std::string, Fn>> m_FlowFns;  // ordered
    std::vector<std::pair<std::string, Fn>> m_ProbeFns; // KPI observation
    std::vector<Event> m_Events;
    std::map<std::string, float> m_Aux;                 // current step
    std::map<std::string, float> m_AuxPrev;             // previous step
    std::map<std::string, std::vector<float>> m_History;
    std::map<std::string, std::vector<float>> m_Probes;
    std::mt19937 m_Rng{42};  // fixed default seed | 固定預設種子
    float m_Dt = 1.f;
    int m_Month = 0;
};

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
