// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/economic/SystemDynamics.h"

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

/** Inputs and response weights for the monthly government legitimacy model. */
struct PoliticalLegitimacyConfig {
    float initialLegitimacy = 0.5f;
    float publicServiceSatisfaction = 0.5f;
    float taxBurden = 0.5f;
    float institutionalCohesion = 0.5f;
    float adjustmentRatePerMonth = 0.1f;
    float publicServiceWeight = 0.4f;
    float taxBurdenWeight = 0.3f;
    float institutionalCohesionWeight = 0.3f;
};

/**
 * Build a bounded monthly legitimacy stock. Its target is the weighted mean
 * of service satisfaction, (1 - tax burden), and institutional cohesion.
 */
Economic::SDModel BuildPoliticalLegitimacyModel(
    const PoliticalLegitimacyConfig& config);

const char* GovernmentLegitimacyStockName();
const char* GovernmentLegitimacyTargetAuxName();
const char* PublicServiceSatisfactionParameterName();
const char* TaxBurdenParameterName();
const char* InstitutionalCohesionParameterName();
const char* LegitimacyAdjustmentRateParameterName();

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
