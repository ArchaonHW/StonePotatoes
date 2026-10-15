// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
// Derived from PotatoEngine Simulation/SystemDynamics.cpp (MIT). See THIRD_PARTY_NOTICES.md.
// 衍生自 PotatoEngine Simulation/SystemDynamics.cpp（MIT），詳見 THIRD_PARTY_NOTICES.md。
#include "SystemDynamics.h"

#include <algorithm>
#include <utility>

namespace StonePotatoes {
namespace Civilization {
namespace Economic {

float SDState::S(const std::string& name) const {
    auto it = m_Stocks->find(name);
    return it != m_Stocks->end() ? it->second : 0.f;
}
float SDState::P(const std::string& name) const {
    auto it = m_Params->find(name);
    return it != m_Params->end() ? it->second : 0.f;
}
float SDState::A(const std::string& name) const {
    auto it = m_Aux->find(name);
    return it != m_Aux->end() ? it->second : 0.f;
}
float SDState::APrev(const std::string& name) const {
    auto it = m_AuxPrev->find(name);
    return it != m_AuxPrev->end() ? it->second : 0.f;
}

void SDModel::AddStock(const std::string& name, float initial, float lo, float hi) {
    auto& s = m_Stocks[name];
    s.value = std::clamp(initial, lo, hi);
    s.lo = lo;
    s.hi = hi;
    if (m_History[name].empty())
        m_History[name].push_back(s.value);
}

void SDModel::SetParam(const std::string& name, float value) {
    m_Params[name] = value;
}

void SDModel::AddAux(const std::string& name, Fn fn) {
    m_AuxFns.emplace_back(name, std::move(fn));
}

void SDModel::AddFlow(const std::string& stock, Fn fn) {
    m_FlowFns.emplace_back(stock, std::move(fn));
}

void SDModel::AddEvent(int month, const std::string& stock, float delta) {
    m_Events.push_back({month, stock, delta, 1.f});
}

void SDModel::AddRandomEvent(int month, const std::string& stock, float delta, float prob) {
    m_Events.push_back({month, stock, delta, std::clamp(prob, 0.f, 1.f)});
}

void SDModel::SetSeed(uint32_t seed) {
    m_Rng.seed(seed);
}

void SDModel::AddDelayChain(const std::string& name, Fn src, int stages, float tau) {
    stages = std::max(1, stages);
    const float stageTau = tau / stages;
    // Each stage: out = prevSelf + (input − prevSelf)/stageTau
    // Stage 1 input = src(); stage i>1 input = A(previous stage current value)
    std::string prev; // empty = eat src
    for (int i = 1; i <= stages; ++i) {
        const std::string stageName = (i == stages) ? name : name + "#" + std::to_string(i);
        if (prev.empty()) {
            AddAux(stageName, [src, stageTau, stageName](const SDState& s) {
                const float cur = s.APrev(stageName);
                return cur + (src(s) - cur) / stageTau;
            });
        } else {
            const std::string prevStage = prev;
            AddAux(stageName, [stageTau, stageName, prevStage](const SDState& s) {
                const float cur = s.APrev(stageName);
                return cur + (s.A(prevStage) - cur) / stageTau;
            });
        }
        prev = stageName;
    }
}

void SDModel::Step() {
    // Build read-only state view (flows all see t state)
    std::map<std::string, float> stockView;
    for (const auto& [n, s] : m_Stocks)
        stockView[n] = s.value;

    SDState st;
    st.m_Stocks = &stockView;
    st.m_Params = &m_Params;
    st.m_Aux = &m_Aux;
    st.m_AuxPrev = &m_AuxPrev;
    st.m_Month = m_Month;

    // 1. aux evaluated in registration order (aux can reference earlier-registered aux)
    //    m_AuxPrev must be moved before evaluation (otherwise APrev lags two steps)
    m_AuxPrev = m_Aux;
    std::map<std::string, float> newAux;
    SDState ast = st;
    ast.m_Aux = &newAux;
    for (const auto& [name, fn] : m_AuxFns)
        newAux[name] = fn ? fn(ast) : 0.f;
    m_Aux = std::move(newAux);
    st.m_Aux = &m_Aux;

    // 2. all flows evaluated at t synchronously (Euler: compute all then apply)
    std::map<std::string, float> deltas;
    for (const auto& [stock, fn] : m_FlowFns)
        deltas[stock] += fn ? fn(st) : 0.f;

    // 3. discrete event injection (month-triggered; prob<1 uses RNG).
    // Events are one-time stock impulses, so keep them separate from rates
    // that are integrated over dt.
    std::uniform_real_distribution<float> uni(0.f, 1.f);
    std::map<std::string, float> eventDeltas;
    for (const auto& e : m_Events)
        if (e.month == m_Month && (e.prob >= 1.f || uni(m_Rng) < e.prob))
            eventDeltas[e.stock] += e.delta;

    // 4. apply + CLAMP + record
    ++m_Month;
    for (auto& [name, s] : m_Stocks) {
        s.value = std::clamp(s.value + m_Dt * deltas[name] + eventDeltas[name],
                             s.lo, s.hi);
        m_History[name].push_back(s.value);
    }

    // 5. KPI probe recording (current step state — aux/stocks are t+1)
    for (const auto& [n, s] : m_Stocks)
        stockView[n] = s.value; // rebuild t+1 view
    st.m_Month = m_Month;
    for (const auto& [name, fn] : m_ProbeFns)
        m_Probes[name].push_back(fn ? fn(st) : 0.f);
}

void SDModel::Run(int steps) {
    for (int i = 0; i < steps; ++i)
        Step();
}

float SDModel::Stock(const std::string& name) const {
    auto it = m_Stocks.find(name);
    return it != m_Stocks.end() ? it->second.value : 0.f;
}

float SDModel::Aux(const std::string& name) const {
    auto it = m_Aux.find(name);
    return it != m_Aux.end() ? it->second : 0.f;
}

const std::vector<float>& SDModel::History(const std::string& name) const {
    static const std::vector<float> empty;
    auto it = m_History.find(name);
    return it != m_History.end() ? it->second : empty;
}

void SDModel::AddProbe(const std::string& name, Fn fn) {
    m_ProbeFns.emplace_back(name, std::move(fn));
}

const std::vector<float>& SDModel::ProbeHistory(const std::string& name) const {
    static const std::vector<float> empty;
    auto it = m_Probes.find(name);
    return it != m_Probes.end() ? it->second : empty;
}

std::vector<int> SDModel::BreachMonths(const std::string& probe, float threshold) const {
    std::vector<int> out;
    const auto& h = ProbeHistory(probe);
    float prev = -std::numeric_limits<float>::infinity();
    for (size_t i = 0; i < h.size(); ++i) {
        if (h[i] > threshold && prev <= threshold)
            out.push_back(static_cast<int>(i) + 1); // recorded month = step+1
        prev = h[i];
    }
    return out;
}

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
