// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/economic/SystemDynamics.h"

namespace StonePotatoes {
namespace Civilization {
namespace Military {

/** Configuration for a monthly troop and supply stock model. */
struct MilitaryLogisticsConfig {
    float initialTroops = 0.0f;
    float initialSupplies = 0.0f;
    float monthlyRecruitment = 0.0f;
    float monthlySupplyProduction = 0.0f;
    // Supply units consumed by one troop in one month.
    float supplyPerTroopPerMonth = 1.0f;
    // Fractions of troops lost per month, each in [0, 1].
    float baseAttritionRatePerMonth = 0.0f;
    float shortageAttritionRatePerMonth = 0.0f;
};

/** Build a deterministic monthly model; spending is capped by available supply. */
Economic::SDModel BuildMilitaryLogisticsModel(
    const MilitaryLogisticsConfig& config);

const char* MilitaryTroopStockName();
const char* MilitarySupplyStockName();
const char* MonthlyRecruitmentParameterName();
const char* MonthlySupplyProductionParameterName();
const char* SupplyPerTroopParameterName();
const char* BaseAttritionRateParameterName();
const char* ShortageAttritionRateParameterName();

} // namespace Military
} // namespace Civilization
} // namespace StonePotatoes
