// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
// Derived from PotatoEngine Core/Profiler.cpp (MIT). See THIRD_PARTY_NOTICES.md.
// 衍生自 PotatoEngine Core/Profiler.cpp（MIT），詳見 THIRD_PARTY_NOTICES.md。
#include "profiling/Profiler.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <thread>

namespace StonePotatoes {
namespace Core {

namespace {
std::atomic<Profiler*> g_Active{nullptr};

// std::thread::id -> uint32 for the chrome-trace tid field
uint32_t TidHash() {
    return static_cast<uint32_t>(
        std::hash<std::thread::id>{}(std::this_thread::get_id()));
}
} // namespace

Profiler::Profiler(size_t eventCapacity)
    : m_EventCapacity(eventCapacity), m_T0(std::chrono::steady_clock::now()) {
    m_Events.resize(eventCapacity);
}

Profiler::~Profiler() {
    // Clear the global slot if it points at us | 若自己是全域啟用點則清掉
    Profiler* expected = this;
    g_Active.compare_exchange_strong(expected, nullptr);
}

double Profiler::NowUs() const {
    return std::chrono::duration<double, std::micro>(
               std::chrono::steady_clock::now() - m_T0)
        .count();
}

void Profiler::SetActive(Profiler* p) { g_Active.store(p, std::memory_order_release); }
Profiler* Profiler::Current() { return g_Active.load(std::memory_order_acquire); }

uint32_t Profiler::ZoneId(const char* name) {
    std::lock_guard<std::mutex> lk(m_Mutex);
    auto it = m_ZoneIds.find(name);
    if (it != m_ZoneIds.end()) return it->second;
    const uint32_t id = static_cast<uint32_t>(m_ZoneNames.size());
    m_ZoneIds.emplace(name, id);
    m_ZoneNames.emplace_back(name);
    m_Stats.emplace_back();
    m_Stats.back().name = name;
    return id;
}

void Profiler::Record(uint32_t zoneId, double startUs, double durUs) {
    std::lock_guard<std::mutex> lk(m_Mutex);
    if (zoneId >= m_Stats.size()) return;

    ZoneStats& s = m_Stats[zoneId];
    ++s.count;
    s.totalUs += durUs;
    s.lastUs = durUs;
    if (s.count == 1 || durUs < s.minUs) s.minUs = durUs;
    if (durUs > s.maxUs) s.maxUs = durUs;

    if (m_EventCapacity > 0) {
        m_Events[m_EventHead] = {zoneId, TidHash(), startUs, durUs};
        m_EventHead = (m_EventHead + 1) % m_EventCapacity;
        if (m_EventCount < m_EventCapacity) ++m_EventCount;
    }
}

std::vector<ZoneStats> Profiler::GetStats() const {
    std::lock_guard<std::mutex> lk(m_Mutex);
    std::vector<ZoneStats> out = m_Stats;
    std::sort(out.begin(), out.end(),
              [](const ZoneStats& a, const ZoneStats& b) { return a.totalUs > b.totalUs; });
    return out;
}

std::string Profiler::StatsTable() const {
    auto snap = GetStats();
    std::string out;
    char line[256];
    std::snprintf(line, sizeof(line), "%-32s %8s %12s %12s %12s %12s\n",
                  "Zone", "Count", "Total(ms)", "Avg(us)", "Min(us)", "Max(us)");
    out += line;
    for (const auto& s : snap) {
        std::snprintf(line, sizeof(line), "%-32s %8llu %12.3f %12.2f %12.2f %12.2f\n",
                      s.name.c_str(), static_cast<unsigned long long>(s.count),
                      s.totalUs / 1000.0, s.AvgUs(), s.minUs, s.maxUs);
        out += line;
    }
    return out;
}

bool Profiler::DumpChromeTrace(const std::string& path) const {
    std::vector<TraceEvent> snap;
    std::vector<std::string> names;
    {
        std::lock_guard<std::mutex> lk(m_Mutex);
        snap.reserve(m_EventCount);
        if (m_EventCapacity > 0) {
            // ring -> chronological (oldest first) | ring → 時間序
            const size_t start = (m_EventHead + m_EventCapacity - m_EventCount) % m_EventCapacity;
            for (size_t i = 0; i < m_EventCount; ++i)
                snap.push_back(m_Events[(start + i) % m_EventCapacity]);
        }
        names = m_ZoneNames;
    }

    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;

    f << "{\"traceEvents\":[";
    bool first = true;
    char buf[320];
    for (const auto& e : snap) {
        const char* name = (e.zoneId < names.size()) ? names[e.zoneId].c_str() : "?";
        // Minimal JSON escape (zone names are string literals) | 基本跳脫
        std::string esc;
        for (const char* c = name; *c; ++c) {
            if (*c == '"' || *c == '\\') esc += '\\';
            esc += *c;
        }
        int n = std::snprintf(buf, sizeof(buf),
            "%s{\"name\":\"%s\",\"ph\":\"X\",\"ts\":%.3f,\"dur\":%.3f,\"pid\":1,\"tid\":%u}",
            first ? "" : ",", esc.c_str(), e.tsUs, e.durUs, e.tidHash);
        if (n > 0) f.write(buf, std::min<int>(n, static_cast<int>(sizeof(buf)) - 1));
        first = false;
    }
    f << "]}\n";
    return f.good();
}

void Profiler::Reset() {
    std::lock_guard<std::mutex> lk(m_Mutex);
    for (auto& s : m_Stats) {
        s.count = 0;
        s.totalUs = s.minUs = s.maxUs = s.lastUs = 0.0;
    }
    m_EventHead = 0;
    m_EventCount = 0;
}

ScopedProfileZone::ScopedProfileZone(const char* name)
    : m_Profiler(Profiler::Current()), m_Zone(0), m_StartUs(0.0) {
    if (m_Profiler) {
        m_Zone = m_Profiler->ZoneId(name);
        m_StartUs = m_Profiler->NowUs();
    }
}

ScopedProfileZone::~ScopedProfileZone() {
    if (m_Profiler) m_Profiler->Record(m_Zone, m_StartUs, m_Profiler->NowUs() - m_StartUs);
}

} // namespace Core
} // namespace StonePotatoes
