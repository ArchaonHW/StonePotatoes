// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "civilization/military/MilitaryLogistics.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Military {
namespace {

float NonnegativeFinite(float value) {
    return std::isfinite(value) ? std::max(0.0f, value) : 0.0f;
}

float UnitRate(float value) {
    return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : 0.0f;
}

double AvailableSupplies(const Economic::SDState& state) {
    return static_cast<double>(NonnegativeFinite(
               state.S(MilitarySupplyStockName()))) +
           NonnegativeFinite(state.P(MonthlySupplyProductionParameterName()));
}

double MonthlySupplyDemand(const Economic::SDState& state) {
    return static_cast<double>(NonnegativeFinite(
               state.S(MilitaryTroopStockName()))) *
           NonnegativeFinite(state.P(SupplyPerTroopParameterName()));
}

} // namespace

const char* MilitaryTroopStockName() { return "military_troops"; }
const char* MilitarySupplyStockName() { return "military_supplies"; }
const char* MonthlyRecruitmentParameterName() {
    return "military_monthly_recruitment";
}
const char* MonthlySupplyProductionParameterName() {
    return "military_monthly_supply_production";
}
const char* SupplyPerTroopParameterName() {
    return "military_supply_per_troop_per_month";
}
const char* BaseAttritionRateParameterName() {
    return "military_base_attrition_rate_per_month";
}
const char* ShortageAttritionRateParameterName() {
    return "military_shortage_attrition_rate_per_month";
}

Economic::SDModel BuildMilitaryLogisticsModel(
    const MilitaryLogisticsConfig& config) {
    if (!std::isfinite(config.initialTroops) || config.initialTroops < 0.0f ||
        !std::isfinite(config.initialSupplies) || config.initialSupplies < 0.0f ||
        !std::isfinite(config.monthlyRecruitment) ||
        config.monthlyRecruitment < 0.0f ||
        !std::isfinite(config.monthlySupplyProduction) ||
        config.monthlySupplyProduction < 0.0f ||
        !std::isfinite(config.supplyPerTroopPerMonth) ||
        config.supplyPerTroopPerMonth < 0.0f ||
        !std::isfinite(config.baseAttritionRatePerMonth) ||
        config.baseAttritionRatePerMonth < 0.0f ||
        config.baseAttritionRatePerMonth > 1.0f ||
        !std::isfinite(config.shortageAttritionRatePerMonth) ||
        config.shortageAttritionRatePerMonth < 0.0f ||
        config.shortageAttritionRatePerMonth > 1.0f) {
        throw std::invalid_argument("invalid military logistics configuration");
    }

    Economic::SDModel model;
    const float maxStock = std::numeric_limits<float>::max();
    model.AddStock(MilitaryTroopStockName(), config.initialTroops, 0.0f,
                   maxStock);
    model.AddStock(MilitarySupplyStockName(), config.initialSupplies, 0.0f,
                   maxStock);
    model.SetParam(MonthlyRecruitmentParameterName(),
                   config.monthlyRecruitment);
    model.SetParam(MonthlySupplyProductionParameterName(),
                   config.monthlySupplyProduction);
    model.SetParam(SupplyPerTroopParameterName(),
                   config.supplyPerTroopPerMonth);
    model.SetParam(BaseAttritionRateParameterName(),
                   config.baseAttritionRatePerMonth);
    model.SetParam(ShortageAttritionRateParameterName(),
                   config.shortageAttritionRatePerMonth);

    model.AddFlow(MilitarySupplyStockName(), [](const Economic::SDState& state) {
        const double demand = MonthlySupplyDemand(state);
        const double available = AvailableSupplies(state);
        const double consumption = std::min(demand, available);
        const double production = NonnegativeFinite(
            state.P(MonthlySupplyProductionParameterName()));
        return static_cast<float>(production - consumption);
    });

    model.AddFlow(MilitaryTroopStockName(), [](const Economic::SDState& state) {
        const float troops = NonnegativeFinite(
            state.S(MilitaryTroopStockName()));
        const double demand = MonthlySupplyDemand(state);
        const double available = AvailableSupplies(state);
        const float served = demand <= 0.0
            ? 1.0f
            : static_cast<float>(std::clamp(available / demand, 0.0, 1.0));
        const double baseLoss = static_cast<double>(troops) *
            UnitRate(state.P(BaseAttritionRateParameterName()));
        const double shortageLoss = static_cast<double>(troops) *
            UnitRate(state.P(ShortageAttritionRateParameterName())) *
            (1.0f - served);
        const double recruitment = NonnegativeFinite(
            state.P(MonthlyRecruitmentParameterName()));
        const double netChange = recruitment - baseLoss - shortageLoss;
        const double limit = std::numeric_limits<float>::max();
        return static_cast<float>(std::clamp(netChange, -limit, limit));
    });
    return model;
}

} // namespace Military
} // namespace Civilization
} // namespace StonePotatoes
