// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
// Unit tests for PotatoEngine ports: Profiler, Pathfinding, QuasiRandom, CurlNoise,
// SystemDynamics, LockstepSync.
// 移植自 PotatoEngine 的工具單元測試。

#include "core/profiling/Profiler.h"
#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Pathfinding.h"
#include "math/QuasiRandom.h"
#include "math/CurlNoise.h"
#include "civilization/economic/SystemDynamics.h"
#include "civilization/population/PopulationDynamics.h"
#include "networking/core/LockstepSync.h"

#include <cstdio>
#include <cmath>
#include <stdexcept>

using namespace StonePotatoes;

static int g_Passed = 0;
static int g_Failed = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (cond) { ++g_Passed; }                                          \
        else {                                                             \
            ++g_Failed;                                                    \
            std::printf("[FAIL] %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
        }                                                                  \
    } while (0)

// ---- Profiler tests ----
static void TestProfiler() {
    using namespace Core;
    Profiler profiler(64);
    Profiler::SetActive(&profiler);

    {
        STONEPOTATOES_PROFILE_SCOPE("TestZone");
        for (volatile int i = 0; i < 1000; ++i) {}
    }

    Profiler::SetActive(nullptr);
    auto stats = profiler.GetStats();
    CHECK(stats.size() == 1);
    CHECK(stats[0].name == "TestZone");
    CHECK(stats[0].count == 1);
    CHECK(stats[0].totalUs > 0.0);
}

// ---- Vec2/Vec3 tests ----
static void TestVec() {
    using namespace Math;
    Vec2 a(3.0f, 4.0f);
    CHECK(std::abs(a.Length() - 5.0f) < 1e-5f);
    Vec2 b = a.Normalize();
    CHECK(std::abs(b.Length() - 1.0f) < 1e-5f);

    Vec3 c(1.0f, 2.0f, 2.0f);
    CHECK(std::abs(c.Length() - 3.0f) < 1e-5f);
}

// ---- Pathfinding tests ----
static void TestPathfinding() {
    using namespace Math;
    GridPathfinder pf(10, 10);
    pf.SetWalkable(5, 5, false); // obstacle

    auto path = pf.FindPath(0, 0, 9, 9);
    CHECK(!path.empty());
    CHECK(path.front().first == 0 && path.front().second == 0);
    CHECK(path.back().first == 9 && path.back().second == 9);

    FlowField ff(10, 10);
    ff.SetWalkable(5, 5, false);
    CHECK(ff.Compute(9, 9));
    float dx, dy;
    CHECK(ff.Direction(8, 8, dx, dy));
}

// ---- QuasiRandom tests ----
static void TestQuasiRandom() {
    using namespace Math::Quasi;
    std::vector<Math::Vec2> points;
    for (uint64_t i = 0; i < 100; ++i) points.push_back(R2(i));
    double disc = Discrepancy2D(points, 8);
    // Quasi-random should have lower discrepancy than PRNG
    CHECK(disc < 0.15);

    auto disk = PoissonDisk(Math::Vec2(0.5f, 0.5f), 0.4f, 20, 0.08f, 42);
    CHECK(disk.size() > 0);
    CHECK(disk.size() <= 20);
}

// ---- CurlNoise tests ----
static void TestCurlNoise() {
    using namespace Math::Quasi;
    TurbulenceField field(4, 123);
    Math::Vec3 p(0.0f, 0.0f, 0.0f);
    Math::Vec3 v = field.Sample(p, 0.0f);
    CHECK(std::abs(field.EstimateDivergence(p, 0.0f)) < 1e-4);
}

// ---- SystemDynamics tests ----
static void TestSystemDynamics() {
    using namespace Civilization::Economic;
    SDModel model;
    model.AddStock("cash", 1000.0f);
    model.SetParam("income", 100.0f);
    model.AddFlow("cash", [](const SDState& s){ return s.P("income"); });
    model.Run(12);
    CHECK(std::abs(model.Stock("cash") - 2200.0f) < 1e-3f); // 1000 + 12*100

    CHECK(model.History("cash").size() == 13); // initial + 12 steps

    SDModel eventModel;
    eventModel.AddStock("treasury", 10.0f);
    eventModel.SetDt(0.5f);
    eventModel.AddEvent(0, "treasury", -3.0f);
    eventModel.Step();
    CHECK(std::abs(eventModel.Stock("treasury") - 7.0f) < 1e-3f);
    eventModel.Step();
    CHECK(std::abs(eventModel.Stock("treasury") - 7.0f) < 1e-3f);
}

static void TestPopulationDynamics() {
    using namespace Civilization::Population;

    auto growing = BuildPopulationModel({100.0f, 1000.0f, 0.2f});
    growing.Step();
    CHECK(std::abs(growing.Stock("population") - 118.0f) < 1e-3f);
    CHECK(growing.History("population").size() == 2);
    growing.SetParam("intrinsic_growth_rate_per_month", -0.2f);
    growing.Step();
    CHECK(growing.Stock("population") < 118.0f);

    auto capped = BuildPopulationModel({999.0f, 1000.0f, 1000.0f});
    capped.Step();
    CHECK(std::abs(capped.Stock("population") - 1000.0f) < 1e-3f);
    capped.Step();
    CHECK(std::abs(capped.Stock("population") - 1000.0f) < 1e-3f);

    auto empty = BuildPopulationModel({0.0f, 1000.0f, 0.2f});
    empty.Step();
    CHECK(empty.Stock("population") == 0.0f);

    bool rejectedInvalidCapacity = false;
    try {
        (void)BuildPopulationModel({100.0f, 0.0f, 0.2f});
    } catch (const std::invalid_argument&) {
        rejectedInvalidCapacity = true;
    }
    CHECK(rejectedInvalidCapacity);
}

// ---- LockstepSync tests ----
static void TestLockstepSync() {
    using namespace Networking;
    LockstepSync::Config cfg;
    cfg.numPeers = 2;
    cfg.localPeerId = 0;
    cfg.inputDelayTicks = 2;
    LockstepSync sync(cfg);

    sync.QueueLocalInput(std::vector<uint8_t>{1, 2, 3});
    auto pkts = sync.SealLocalPackets();
    CHECK(pkts.size() > 0);

    // Simulate peer 1 packets for ticks 0,1,2 (pre-filled and current)
    for (int t = 0; t <= 2; ++t) {
        auto pkt = LockstepSync::EncodeCommandPacket(1, t, {});
        sync.OnPeerPacket(pkt);
    }

    CHECK(sync.ReadyToAdvance());

    LockstepSync::TickCommands tc;
    CHECK(sync.TryAdvance(tc));
    CHECK(tc.tick == 0);
}

int main() {
    TestProfiler();
    TestVec();
    TestPathfinding();
    TestQuasiRandom();
    TestCurlNoise();
    TestSystemDynamics();
    TestPopulationDynamics();
    TestLockstepSync();

    std::printf("\n=== Ported Utils Tests ===\n");
    std::printf("Passed: %d\n", g_Passed);
    std::printf("Failed: %d\n", g_Failed);
    return g_Failed == 0 ? 0 : 1;
}
