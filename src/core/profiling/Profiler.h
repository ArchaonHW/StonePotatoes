// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
// Derived from PotatoEngine Core/Profiler.{h,cpp} (MIT). See THIRD_PARTY_NOTICES.md.
// Adapted: StonePotatoes namespace, STONEPOTATOES_PROFILE_SCOPE macro.
// 衍生自 PotatoEngine Core/Profiler.{h,cpp}（MIT），詳見 THIRD_PARTY_NOTICES.md。
// 改寫：StonePotatoes 命名空間、STONEPOTATOES_PROFILE_SCOPE 巨集。
#pragma once

// Profiler — scoped zone timing + Chrome trace export.
//   1) Aggregate stats per zone: count/total/avg/min/max (StatsTable/GetStats)
//   2) Fixed-capacity event ring; DumpChromeTrace writes JSON readable by
//      chrome://tracing / Perfetto.
// STONEPOTATOES_PROFILE_SCOPE("Zone") is near-free while no profiler is
// active (one atomic load + branch in ctor/dtor). A single short mutex
// guards aggregation — designed for tens to thousands of zones per frame,
// not microsecond-frequency zones.
// Profiler — 區段計時與追蹤匯出。
//   1) 聚合統計：每 zone 的 count/total/avg/min/max
//   2) 事件環：固定容量 ring buffer，DumpChromeTrace 匯出
//      chrome://tracing / Perfetto 可開的 JSON
// 未啟用時巨集近乎免費（ctor/dtor 各一次原子讀 + 分支）。
// 單 mutex 聚合，設計給每幀數十~數千次的區段。

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace StonePotatoes {
namespace Core {

// Per-zone aggregate statistics (microseconds) | 單一 zone 聚合統計（微秒）
struct ZoneStats {
    std::string name;
    uint64_t count = 0;
    double totalUs = 0.0;
    double minUs = 0.0;
    double maxUs = 0.0;
    double lastUs = 0.0;
    double AvgUs() const { return count > 0 ? totalUs / count : 0.0; }
};

class Profiler {
public:
    // eventCapacity: trace ring size (~32 B/event) | 事件環容量
    explicit Profiler(size_t eventCapacity = 65536);
    ~Profiler();

    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    // Intern a zone name; same name -> same id | 名稱駐留回 zone id
    uint32_t ZoneId(const char* name);
    // Record one sample (ts relative to construction) | 記一筆
    void Record(uint32_t zoneId, double startUs, double durUs);

    // Snapshot sorted by totalUs desc | 依 totalUs 降冪的快照
    std::vector<ZoneStats> GetStats() const;
    // Fixed-width text table | 固定欄寬表格
    std::string StatsTable() const;
    // {"traceEvents":[...]} with ph="X" complete events
    bool DumpChromeTrace(const std::string& path) const;
    void Reset();

    // Global activation point read by ScopedProfileZone | 全域啟用點
    static void SetActive(Profiler* p);
    static Profiler* Current();

    // Monotonic microseconds since construction | 單調時鐘相對微秒
    double NowUs() const;

private:
    struct TraceEvent {
        uint32_t zoneId;
        uint32_t tidHash;
        double tsUs;
        double durUs;
    };

    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, uint32_t> m_ZoneIds;
    std::vector<std::string> m_ZoneNames;
    std::vector<ZoneStats> m_Stats;

    std::vector<TraceEvent> m_Events;   // ring buffer
    size_t m_EventHead = 0;
    size_t m_EventCount = 0;
    size_t m_EventCapacity;

    const std::chrono::steady_clock::time_point m_T0;
};

// RAII zone timer; records nothing when Current() is null.
// RAII 區段計時器：Current()==nullptr 時不記錄。
class ScopedProfileZone {
public:
    explicit ScopedProfileZone(const char* name);
    ~ScopedProfileZone();

    ScopedProfileZone(const ScopedProfileZone&) = delete;
    ScopedProfileZone& operator=(const ScopedProfileZone&) = delete;

private:
    Profiler* m_Profiler;   // snapshot at construction | 建構時快照
    uint32_t m_Zone;
    double m_StartUs;
};

} // namespace Core
} // namespace StonePotatoes

#define STONEPOTATOES_PP_CAT_(a, b) a##b
#define STONEPOTATOES_PP_CAT(a, b) STONEPOTATOES_PP_CAT_(a, b)
#define STONEPOTATOES_PROFILE_SCOPE(name) \
    ::StonePotatoes::Core::ScopedProfileZone STONEPOTATOES_PP_CAT(_spz_, __LINE__)(name)
