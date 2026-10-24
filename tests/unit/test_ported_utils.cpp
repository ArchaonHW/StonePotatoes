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
#include "civilization/economic/MarketPrice.h"
#include "civilization/economic/FiscalBudget.h"
#include "civilization/economic/TradeNetwork.h"
#include "civilization/culture/CultureDiffusion.h"
#include "civilization/population/PopulationDynamics.h"
#include "networking/core/LockstepSync.h"

#include <cstdio>
#include <cmath>
#include <limits>
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

    auto immigration = BuildPopulationModel({100.0f, 1000.0f, 0.0f, 25.0f});
    immigration.Step();
    CHECK(std::abs(immigration.Stock("population") - 125.0f) < 1e-3f);
    immigration.SetParam("net_migration_per_month", -10.0f);
    immigration.Step();
    CHECK(std::abs(immigration.Stock("population") - 115.0f) < 1e-3f);

    auto boundedMigration =
        BuildPopulationModel({990.0f, 1000.0f, 0.0f, 25.0f});
    boundedMigration.Step();
    CHECK(boundedMigration.Stock("population") == 1000.0f);

    auto demographic = BuildDemographicPopulationModel(
        {100.0f, 1000.0f, 0.02f, 0.005f, 0.0f});
    demographic.Step();
    CHECK(std::abs(demographic.Stock("population") - 101.3f) < 1e-3f);
    demographic.SetParam("birth_rate_per_month", 0.0f);
    demographic.SetParam("death_rate_per_month", 0.01f);
    demographic.Step();
    CHECK(std::abs(demographic.Stock("population") - 100.287f) < 1e-3f);

    auto fullDemographic = BuildDemographicPopulationModel(
        {999.0f, 1000.0f, 10.0f, 0.0f, 0.0f});
    fullDemographic.Step();
    CHECK(fullDemographic.Stock("population") == 1000.0f);

    bool rejectedInvalidCapacity = false;
    try {
        (void)BuildPopulationModel({100.0f, 0.0f, 0.2f});
    } catch (const std::invalid_argument&) {
        rejectedInvalidCapacity = true;
    }
    CHECK(rejectedInvalidCapacity);

    bool rejectedNonFiniteMigration = false;
    try {
        (void)BuildPopulationModel(
            {100.0f, 1000.0f, 0.2f,
             std::numeric_limits<float>::infinity()});
    } catch (const std::invalid_argument&) {
        rejectedNonFiniteMigration = true;
    }
    CHECK(rejectedNonFiniteMigration);

    bool rejectedNegativeBirthRate = false;
    try {
        (void)BuildDemographicPopulationModel(
            {100.0f, 1000.0f, -0.01f, 0.005f, 0.0f});
    } catch (const std::invalid_argument&) {
        rejectedNegativeBirthRate = true;
    }
    CHECK(rejectedNegativeBirthRate);

    RegionalPopulationConfig regionalConfig;
    regionalConfig.initialPopulationByRegion = {100.0f, 50.0f};
    regionalConfig.carryingCapacityByRegion = {1000.0f, 1000.0f};
    regionalConfig.intrinsicGrowthRatePerMonthByRegion = {0.0f, 0.0f};
    regionalConfig.monthlyMigrationFromRegionToRegion = {
        {0.0f, 20.0f}, {5.0f, 0.0f}};
    auto regional = BuildRegionalPopulationModel(regionalConfig);
    regional.SetParam(RegionMigrationParameterName(0, 0), 500.0f);
    regional.Step();
    CHECK(std::abs(regional.Stock(RegionPopulationStockName(0)) - 85.0f) <
          1e-3f);
    CHECK(std::abs(regional.Stock(RegionPopulationStockName(1)) - 65.0f) <
          1e-3f);
    CHECK(std::abs(regional.Stock(RegionPopulationStockName(0)) +
                   regional.Stock(RegionPopulationStockName(1)) - 150.0f) <
          1e-3f);
    regional.SetParam(RegionMigrationParameterName(0, 1), 0.0f);
    regional.SetParam(RegionMigrationParameterName(1, 0), 10.0f);
    regional.Step();
    CHECK(std::abs(regional.Stock(RegionPopulationStockName(0)) - 95.0f) <
          1e-3f);
    CHECK(std::abs(regional.Stock(RegionPopulationStockName(1)) - 55.0f) <
          1e-3f);

    regionalConfig.initialPopulationByRegion = {100.0f, 90.0f};
    regionalConfig.carryingCapacityByRegion = {1000.0f, 100.0f};
    regionalConfig.monthlyMigrationFromRegionToRegion = {
        {0.0f, 30.0f}, {0.0f, 0.0f}};
    auto destinationBounded = BuildRegionalPopulationModel(regionalConfig);
    destinationBounded.Step();
    CHECK(std::abs(destinationBounded.Stock(RegionPopulationStockName(0)) -
                   90.0f) < 1e-3f);
    CHECK(destinationBounded.Stock(RegionPopulationStockName(1)) == 100.0f);

    regionalConfig.initialPopulationByRegion = {100.0f, 0.0f};
    regionalConfig.monthlyMigrationFromRegionToRegion = {
        {0.0f, 1000.0f}, {0.0f, 0.0f}};
    auto sourceBounded = BuildRegionalPopulationModel(regionalConfig);
    sourceBounded.Step();
    CHECK(sourceBounded.Stock(RegionPopulationStockName(0)) == 0.0f);
    CHECK(std::abs(sourceBounded.Stock(RegionPopulationStockName(1)) - 100.0f) <
          1e-3f);

    bool rejectedInvalidRegionalShape = false;
    try {
        RegionalPopulationConfig invalid = regionalConfig;
        invalid.monthlyMigrationFromRegionToRegion[0].pop_back();
        (void)BuildRegionalPopulationModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedInvalidRegionalShape = true;
    }
    CHECK(rejectedInvalidRegionalShape);

    bool rejectedNegativeRegionalMigration = false;
    try {
        RegionalPopulationConfig invalid = regionalConfig;
        invalid.monthlyMigrationFromRegionToRegion[0][1] = -1.0f;
        (void)BuildRegionalPopulationModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedNegativeRegionalMigration = true;
    }
    CHECK(rejectedNegativeRegionalMigration);

    AgeStructuredPopulationConfig ageConfig;
    ageConfig.initialPopulationByAgeGroup = {100.0f, 50.0f, 10.0f};
    ageConfig.annualSurvivalRateByAgeGroup = {0.9f, 0.8f, 0.5f};
    ageConfig.annualBirthsPerPersonByAgeGroup = {0.0f, 0.2f, 0.1f};
    ageConfig.carryingCapacity = 1000.0f;
    auto ageStructured = BuildAgeStructuredPopulationModel(ageConfig);
    ageStructured.Step();
    CHECK(std::abs(ageStructured.Stock(AgeGroupStockName(0)) - 11.0f) < 1e-3f);
    CHECK(std::abs(ageStructured.Stock(AgeGroupStockName(1)) - 90.0f) < 1e-3f);
    CHECK(std::abs(ageStructured.Stock(AgeGroupStockName(2)) - 45.0f) < 1e-3f);
    ageStructured.SetParam(AgeGroupSurvivalParameterName(0), 0.0f);
    ageStructured.SetParam(AgeGroupBirthRateParameterName(1), 0.0f);
    ageStructured.Step();
    CHECK(std::abs(ageStructured.Stock(AgeGroupStockName(0)) - 4.5f) < 1e-3f);
    CHECK(ageStructured.Stock(AgeGroupStockName(1)) == 0.0f);
    CHECK(std::abs(ageStructured.Stock(AgeGroupStockName(2)) - 94.5f) < 1e-3f);

    ageConfig.initialPopulationByAgeGroup = {100.0f, 50.0f};
    ageConfig.annualSurvivalRateByAgeGroup = {1.0f, 1.0f};
    ageConfig.annualBirthsPerPersonByAgeGroup = {0.0f, 0.0f};
    ageConfig.annualNetMigrationByAgeGroup = {10.0f, -1000.0f};
    ageConfig.carryingCapacity = 1000.0f;
    auto migrating = BuildAgeStructuredPopulationModel(ageConfig);
    migrating.Step();
    CHECK(std::abs(migrating.Stock(AgeGroupStockName(0)) - 10.0f) < 1e-3f);
    CHECK(std::abs(migrating.Stock(AgeGroupStockName(1))) < 1e-3f);

    ageConfig.annualNetMigrationByAgeGroup = {1000.0f, 1000.0f};
    ageConfig.carryingCapacity = 160.0f;
    auto migrationCapacityBounded = BuildAgeStructuredPopulationModel(ageConfig);
    migrationCapacityBounded.Step();
    CHECK(std::abs(migrationCapacityBounded.Stock(AgeGroupStockName(0)) -
                   5.0f) < 1e-3f);
    CHECK(std::abs(migrationCapacityBounded.Stock(AgeGroupStockName(1)) -
                   155.0f) < 1e-3f);

    ageConfig.initialPopulationByAgeGroup = {100.0f, 0.0f};
    ageConfig.annualSurvivalRateByAgeGroup = {1.0f, 1.0f};
    ageConfig.annualBirthsPerPersonByAgeGroup = {2.0f, 0.0f};
    ageConfig.carryingCapacity = 150.0f;
    auto capacityBounded = BuildAgeStructuredPopulationModel(ageConfig);
    capacityBounded.Step();
    CHECK(capacityBounded.Stock(AgeGroupStockName(0)) == 50.0f);
    CHECK(capacityBounded.Stock(AgeGroupStockName(1)) == 100.0f);

    bool rejectedMismatchedAgeGroups = false;
    try {
        AgeStructuredPopulationConfig invalid = ageConfig;
        invalid.annualBirthsPerPersonByAgeGroup.pop_back();
        (void)BuildAgeStructuredPopulationModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedMismatchedAgeGroups = true;
    }
    CHECK(rejectedMismatchedAgeGroups);

    bool rejectedMismatchedMigration = false;
    try {
        AgeStructuredPopulationConfig invalid = ageConfig;
        invalid.annualNetMigrationByAgeGroup.pop_back();
        (void)BuildAgeStructuredPopulationModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedMismatchedMigration = true;
    }
    CHECK(rejectedMismatchedMigration);

    bool rejectedNonFiniteAgeMigration = false;
    try {
        AgeStructuredPopulationConfig invalid = ageConfig;
        invalid.annualNetMigrationByAgeGroup[0] =
            std::numeric_limits<float>::infinity();
        (void)BuildAgeStructuredPopulationModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedNonFiniteAgeMigration = true;
    }
    CHECK(rejectedNonFiniteAgeMigration);
}

static void TestTradeNetwork() {
    using namespace Civilization::Economic;

    TradeNetworkConfig config;
    config.initialStockByRegion = {100.0f, 20.0f};
    config.storageCapacityByRegion = {500.0f, 500.0f};
    config.monthlyProductionByRegion = {10.0f, 0.0f};
    config.monthlyConsumptionByRegion = {5.0f, 0.0f};
    config.monthlyTradeFromRegionToRegion = {
        {0.0f, 30.0f}, {0.0f, 0.0f}};
    auto trade = BuildTradeNetworkModel(config);
    trade.Step();
    CHECK(std::abs(trade.Stock(TradeStockName(0)) - 75.0f) < 1e-3f);
    CHECK(std::abs(trade.Stock(TradeStockName(1)) - 50.0f) < 1e-3f);
    CHECK(std::abs(trade.Stock(TradeStockName(0)) +
                   trade.Stock(TradeStockName(1)) - 125.0f) < 1e-3f);

    trade.SetParam(TradeRouteParameterName(0, 1), 0.0f);
    trade.SetParam(TradeRouteParameterName(1, 0), 10.0f);
    trade.SetParam(TradeProductionParameterName(1), 5.0f);
    trade.Step();
    CHECK(std::abs(trade.Stock(TradeStockName(0)) - 90.0f) < 1e-3f);
    CHECK(std::abs(trade.Stock(TradeStockName(1)) - 45.0f) < 1e-3f);

    config.initialStockByRegion = {10.0f, 0.0f};
    config.storageCapacityByRegion = {100.0f, 100.0f};
    config.monthlyProductionByRegion = {0.0f, 0.0f};
    config.monthlyConsumptionByRegion = {0.0f, 0.0f};
    config.monthlyTradeFromRegionToRegion = {
        {0.0f, 100.0f}, {0.0f, 0.0f}};
    auto sourceBounded = BuildTradeNetworkModel(config);
    sourceBounded.Step();
    CHECK(sourceBounded.Stock(TradeStockName(0)) == 0.0f);
    CHECK(sourceBounded.Stock(TradeStockName(1)) == 10.0f);

    config.initialStockByRegion = {100.0f, 45.0f};
    config.storageCapacityByRegion = {1000.0f, 50.0f};
    config.monthlyTradeFromRegionToRegion = {
        {0.0f, 25.0f}, {0.0f, 0.0f}};
    auto destinationBounded = BuildTradeNetworkModel(config);
    destinationBounded.Step();
    CHECK(std::abs(destinationBounded.Stock(TradeStockName(0)) - 95.0f) <
          1e-3f);
    CHECK(destinationBounded.Stock(TradeStockName(1)) == 50.0f);

    config.initialStockByRegion = {0.0f, 5.0f};
    config.monthlyTradeFromRegionToRegion = {
        {0.0f, 0.0f}, {0.0f, 0.0f}};
    config.monthlyConsumptionByRegion = {100.0f, 100.0f};
    auto consumptionBounded = BuildTradeNetworkModel(config);
    consumptionBounded.Step();
    CHECK(consumptionBounded.Stock(TradeStockName(0)) == 0.0f);
    CHECK(consumptionBounded.Stock(TradeStockName(1)) == 0.0f);

    bool rejectedInvalidTradeMatrix = false;
    try {
        TradeNetworkConfig invalid = config;
        invalid.monthlyTradeFromRegionToRegion[0].pop_back();
        (void)BuildTradeNetworkModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedInvalidTradeMatrix = true;
    }
    CHECK(rejectedInvalidTradeMatrix);

    bool rejectedSelfTrade = false;
    try {
        TradeNetworkConfig invalid = config;
        invalid.monthlyTradeFromRegionToRegion[0][0] = 1.0f;
        (void)BuildTradeNetworkModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedSelfTrade = true;
    }
    CHECK(rejectedSelfTrade);

    config.initialStockByRegion = {100.0f, 0.0f};
    config.storageCapacityByRegion = {500.0f, 500.0f};
    config.monthlyProductionByRegion = {0.0f, 0.0f};
    config.monthlyConsumptionByRegion = {0.0f, 0.0f};
    config.monthlyTradeFromRegionToRegion = {
        {0.0f, 20.0f}, {0.0f, 0.0f}};
    config.initialFundsByRegion = {5.0f, 0.0f};
    config.monthlyFundsIncomeByRegion = {0.0f, 0.0f};
    config.monthlyTransportCostPerUnitFromRegionToRegion = {
        {0.0f, 0.5f}, {0.0f, 0.0f}};
    auto costBounded = BuildTradeNetworkModel(config);
    costBounded.Step();
    CHECK(std::abs(costBounded.Stock(TradeStockName(0)) - 90.0f) < 1e-3f);
    CHECK(std::abs(costBounded.Stock(TradeStockName(1)) - 10.0f) < 1e-3f);
    CHECK(costBounded.Stock(TradeFundsStockName(0)) == 0.0f);
    CHECK(costBounded.Stock(TradeFundsStockName(1)) == 0.0f);

    costBounded.SetParam(TradeFundsIncomeParameterName(0), 2.0f);
    costBounded.Step();
    CHECK(std::abs(costBounded.Stock(TradeStockName(0)) - 86.0f) < 1e-3f);
    CHECK(std::abs(costBounded.Stock(TradeStockName(1)) - 14.0f) < 1e-3f);
    CHECK(costBounded.Stock(TradeFundsStockName(0)) == 0.0f);

    bool rejectedMismatchedTradeFunds = false;
    try {
        TradeNetworkConfig invalid = config;
        invalid.initialFundsByRegion.pop_back();
        (void)BuildTradeNetworkModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedMismatchedTradeFunds = true;
    }
    CHECK(rejectedMismatchedTradeFunds);

    bool rejectedNegativeTransportCost = false;
    try {
        TradeNetworkConfig invalid = config;
        invalid.monthlyTransportCostPerUnitFromRegionToRegion[0][1] = -1.0f;
        (void)BuildTradeNetworkModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedNegativeTransportCost = true;
    }
    CHECK(rejectedNegativeTransportCost);

    config.initialStockByRegion = {0.0f, 10.0f};
    config.monthlyTradeFromRegionToRegion = {
        {0.0f, 0.0f}, {10.0f, 0.0f}};
    config.initialFundsByRegion = {20.0f, 0.0f};
    config.monthlyFundsIncomeByRegion = {0.0f, 0.0f};
    config.monthlyTransportCostPerUnitFromRegionToRegion.clear();
    config.monthlyTransactionPricePerUnitFromRegionToRegion = {
        {0.0f, 0.0f}, {3.0f, 0.0f}};
    auto priceBounded = BuildTradeNetworkModel(config);
    priceBounded.Step();
    CHECK(std::abs(priceBounded.Stock(TradeStockName(0)) - (20.0f / 3.0f)) <
          1e-3f);
    CHECK(std::abs(priceBounded.Stock(TradeStockName(1)) - (10.0f / 3.0f)) <
          1e-3f);
    CHECK(priceBounded.Stock(TradeFundsStockName(0)) == 0.0f);
    CHECK(std::abs(priceBounded.Stock(TradeFundsStockName(1)) - 20.0f) <
          1e-3f);

    config.initialStockByRegion = {10.0f, 10.0f};
    config.monthlyTradeFromRegionToRegion = {
        {0.0f, 10.0f}, {10.0f, 0.0f}};
    config.initialFundsByRegion = {20.0f, 0.0f};
    config.monthlyTransportCostPerUnitFromRegionToRegion = {
        {0.0f, 1.0f}, {0.0f, 0.0f}};
    config.monthlyTransactionPricePerUnitFromRegionToRegion = {
        {0.0f, 0.0f}, {2.0f, 0.0f}};
    auto combinedSettlement = BuildTradeNetworkModel(config);
    combinedSettlement.Step();
    CHECK(std::abs(combinedSettlement.Stock(TradeStockName(0)) - 5.0f) <
          1e-3f);
    CHECK(std::abs(combinedSettlement.Stock(TradeStockName(1)) - 15.0f) <
          1e-3f);
    CHECK(std::abs(combinedSettlement.Stock(TradeFundsStockName(0)) - 0.0f) <
          1e-3f);
    CHECK(std::abs(combinedSettlement.Stock(TradeFundsStockName(1)) - 10.0f) <
          1e-3f);

    config.initialStockByRegion = {10.0f, 10.0f};
    config.monthlyTradeFromRegionToRegion = {
        {0.0f, 10.0f}, {10.0f, 0.0f}};
    config.initialFundsByRegion = {0.0f, 10.0f};
    config.monthlyTransportCostPerUnitFromRegionToRegion.clear();
    config.monthlyTransactionPricePerUnitFromRegionToRegion = {
        {0.0f, 1.0f}, {1.0f, 0.0f}};
    auto delayedRevenue = BuildTradeNetworkModel(config);
    delayedRevenue.Step();
    CHECK(delayedRevenue.Stock(TradeStockName(0)) == 0.0f);
    CHECK(delayedRevenue.Stock(TradeStockName(1)) == 20.0f);
    CHECK(std::abs(delayedRevenue.Stock(TradeFundsStockName(0)) - 10.0f) <
          1e-3f);

    bool rejectedMismatchedTransactionPriceMatrix = false;
    try {
        TradeNetworkConfig invalid = config;
        invalid.monthlyTransactionPricePerUnitFromRegionToRegion[0].pop_back();
        (void)BuildTradeNetworkModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedMismatchedTransactionPriceMatrix = true;
    }
    CHECK(rejectedMismatchedTransactionPriceMatrix);

    bool rejectedNegativeTransactionPrice = false;
    try {
        TradeNetworkConfig invalid = config;
        invalid.monthlyTransactionPricePerUnitFromRegionToRegion[1][0] = -1.0f;
        (void)BuildTradeNetworkModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedNegativeTransactionPrice = true;
    }
    CHECK(rejectedNegativeTransactionPrice);
}

static void TestMarketPrice() {
    using namespace Civilization::Economic;

    MarketPriceConfig config;
    config.initialPrice = 10.0f;
    config.targetStock = 100.0f;
    config.responsePerMonth = 0.2f;
    config.minimumPrice = 1.0f;
    config.maximumPrice = 100.0f;

    auto scarce = BuildMarketPriceModel(config);
    scarce.SetParam(MarketStockParameterName(), 50.0f);
    scarce.Step();
    CHECK(std::abs(scarce.Stock(MarketPriceStockName()) - 11.0f) < 1e-3f);

    auto abundant = BuildMarketPriceModel(config);
    abundant.SetParam(MarketStockParameterName(), 200.0f);
    abundant.Step();
    CHECK(std::abs(abundant.Stock(MarketPriceStockName()) - 8.0f) < 1e-3f);

    auto balanced = BuildMarketPriceModel(config);
    balanced.Step();
    CHECK(balanced.Stock(MarketPriceStockName()) == 10.0f);

    config.initialPrice = config.minimumPrice;
    config.responsePerMonth = 1.0f;
    auto boundedLow = BuildMarketPriceModel(config);
    boundedLow.SetParam(MarketStockParameterName(), 200.0f);
    boundedLow.Step();
    CHECK(boundedLow.Stock(MarketPriceStockName()) == config.minimumPrice);

    config.initialPrice = config.maximumPrice;
    auto boundedHigh = BuildMarketPriceModel(config);
    boundedHigh.SetParam(MarketStockParameterName(), 0.0f);
    boundedHigh.Step();
    CHECK(boundedHigh.Stock(MarketPriceStockName()) == config.maximumPrice);

    bool rejectedInvalidTarget = false;
    try {
        MarketPriceConfig invalid = config;
        invalid.targetStock = 0.0f;
        (void)BuildMarketPriceModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedInvalidTarget = true;
    }
    CHECK(rejectedInvalidTarget);
}

static void TestFiscalBudget() {
    using namespace Civilization::Economic;

    FiscalBudgetConfig config;
    config.initialTreasury = 0.0f;
    config.monthlyTaxBase = 100.0f;
    config.taxRate = 0.1f;
    config.monthlyPublicSpending = 8.0f;
    auto budget = BuildFiscalBudgetModel(config);
    budget.Step();
    CHECK(std::abs(budget.Stock(FiscalTreasuryStockName()) - 2.0f) <
          1e-4f);

    budget.SetParam(FiscalTaxBaseParameterName(), 0.0f);
    budget.Step();
    CHECK(budget.Stock(FiscalTreasuryStockName()) == 0.0f);

    config.initialTreasury = 5.0f;
    config.monthlyTaxBase = 0.0f;
    config.taxRate = 0.5f;
    config.monthlyPublicSpending = 9.0f;
    auto capped = BuildFiscalBudgetModel(config);
    capped.Step();
    CHECK(capped.Stock(FiscalTreasuryStockName()) == 0.0f);

    config.taxRate = 1.1f;
    bool rejectedInvalidRate = false;
    try {
        (void)BuildFiscalBudgetModel(config);
    } catch (const std::invalid_argument&) {
        rejectedInvalidRate = true;
    }
    CHECK(rejectedInvalidRate);
}

static void TestCultureDiffusion() {
    using namespace Civilization::Culture;

    CultureDiffusionConfig config;
    config.initialTraitsByRegion = {
        {0.0f, 1.0f}, {1.0f, 0.0f}, {0.5f, 0.5f}};
    config.monthlyAssimilationRateFromRegionToRegion = {
        {0.0f, 0.25f, 0.0f},
        {0.25f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}};
    auto culture = BuildCultureDiffusionModel(config);
    culture.Step();
    CHECK(std::abs(culture.Stock(CultureTraitStockName(0, 0)) - 0.25f) <
          1e-3f);
    CHECK(std::abs(culture.Stock(CultureTraitStockName(0, 1)) - 0.75f) <
          1e-3f);
    CHECK(std::abs(culture.Stock(CultureTraitStockName(1, 0)) - 0.75f) <
          1e-3f);
    CHECK(std::abs(culture.Stock(CultureTraitStockName(1, 1)) - 0.25f) <
          1e-3f);
    CHECK(culture.Stock(CultureTraitStockName(2, 0)) == 0.5f);

    // Runtime parameter updates must not let a self-link dilute real
    // incoming influence. Self-links are excluded from the contact matrix,
    // but callers can still update the exposed parameter after construction.
    CultureDiffusionConfig runtimeConfig;
    runtimeConfig.initialTraitsByRegion = {{0.0f}, {1.0f}};
    runtimeConfig.monthlyAssimilationRateFromRegionToRegion = {
        {0.0f, 0.0f}, {0.25f, 0.0f}};
    auto runtimeCulture = BuildCultureDiffusionModel(runtimeConfig);
    runtimeCulture.SetParam(CultureAssimilationParameterName(0, 0), 1.0f);
    runtimeCulture.Step();
    CHECK(std::abs(runtimeCulture.Stock(CultureTraitStockName(0, 0)) -
                   0.25f) < 1e-3f);

    bool rejectedUnboundedIncomingInfluence = false;
    try {
        CultureDiffusionConfig invalid = config;
        invalid.monthlyAssimilationRateFromRegionToRegion[0][2] = 0.75f;
        invalid.monthlyAssimilationRateFromRegionToRegion[1][2] = 0.5f;
        (void)BuildCultureDiffusionModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedUnboundedIncomingInfluence = true;
    }
    CHECK(rejectedUnboundedIncomingInfluence);

    bool rejectedInvalidTrait = false;
    try {
        CultureDiffusionConfig invalid = config;
        invalid.initialTraitsByRegion[0][0] = 1.1f;
        (void)BuildCultureDiffusionModel(invalid);
    } catch (const std::invalid_argument&) {
        rejectedInvalidTrait = true;
    }
    CHECK(rejectedInvalidTrait);
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
    TestTradeNetwork();
    TestMarketPrice();
    TestFiscalBudget();
    TestCultureDiffusion();
    TestLockstepSync();

    std::printf("\n=== Ported Utils Tests ===\n");
    std::printf("Passed: %d\n", g_Passed);
    std::printf("Failed: %d\n", g_Failed);
    return g_Failed == 0 ? 0 : 1;
}
