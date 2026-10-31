// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "PoliticalLegitimacy.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {
namespace {

constexpr char kLegitimacyStock[] = "government_legitimacy";
constexpr char kTargetAux[] = "government_legitimacy_target";
constexpr char kServiceParameter[] = "public_service_satisfaction";
constexpr char kTaxBurdenParameter[] = "tax_burden";
constexpr char kCohesionParameter[] = "institutional_cohesion";
constexpr char kAdjustmentRateParameter[] = "legitimacy_adjustment_rate_per_month";

bool IsUnitInterval(float value) {
    return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
}

} // namespace

Economic::SDModel BuildPoliticalLegitimacyModel(
    const PoliticalLegitimacyConfig& config) {
    const float weightTotal = config.publicServiceWeight +
                              config.taxBurdenWeight +
                              config.institutionalCohesionWeight;
    if (!IsUnitInterval(config.initialLegitimacy) ||
        !IsUnitInterval(config.publicServiceSatisfaction) ||
        !IsUnitInterval(config.taxBurden) ||
        !IsUnitInterval(config.institutionalCohesion) ||
        !IsUnitInterval(config.adjustmentRatePerMonth) ||
        !std::isfinite(config.publicServiceWeight) ||
        config.publicServiceWeight < 0.0f ||
        !std::isfinite(config.taxBurdenWeight) || config.taxBurdenWeight < 0.0f ||
        !std::isfinite(config.institutionalCohesionWeight) ||
        config.institutionalCohesionWeight < 0.0f ||
        !std::isfinite(weightTotal) || weightTotal <= 0.0f) {
        throw std::invalid_argument(
            "political legitimacy inputs must be finite and within their valid ranges");
    }

    const float serviceWeight = config.publicServiceWeight / weightTotal;
    const float taxWeight = config.taxBurdenWeight / weightTotal;
    const float cohesionWeight = config.institutionalCohesionWeight / weightTotal;

    Economic::SDModel model;
    model.SetDt(1.0f);
    model.AddStock(kLegitimacyStock, config.initialLegitimacy, 0.0f, 1.0f);
    model.SetParam(kServiceParameter, config.publicServiceSatisfaction);
    model.SetParam(kTaxBurdenParameter, config.taxBurden);
    model.SetParam(kCohesionParameter, config.institutionalCohesion);
    model.SetParam(kAdjustmentRateParameter, config.adjustmentRatePerMonth);
    model.AddAux(kTargetAux, [serviceWeight, taxWeight, cohesionWeight](
                                 const Economic::SDState& state) {
        const auto factor = [&state](const char* name, float fallback) {
            const float value = state.P(name);
            return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f)
                                        : fallback;
        };
        const float service = factor(kServiceParameter, 0.0f);
        const float taxBurden = factor(kTaxBurdenParameter, 1.0f);
        const float cohesion = factor(kCohesionParameter, 0.0f);
        return serviceWeight * service + taxWeight * (1.0f - taxBurden) +
               cohesionWeight * cohesion;
    });
    model.AddFlow(kLegitimacyStock, [](const Economic::SDState& state) {
        const float rawRate = state.P(kAdjustmentRateParameter);
        const float rate = std::isfinite(rawRate)
            ? std::clamp(rawRate, 0.0f, 1.0f) : 0.0f;
        return rate * (state.A(kTargetAux) - state.S(kLegitimacyStock));
    });
    return model;
}

const char* GovernmentLegitimacyStockName() { return kLegitimacyStock; }
const char* GovernmentLegitimacyTargetAuxName() { return kTargetAux; }
const char* PublicServiceSatisfactionParameterName() { return kServiceParameter; }
const char* TaxBurdenParameterName() { return kTaxBurdenParameter; }
const char* InstitutionalCohesionParameterName() { return kCohesionParameter; }
const char* LegitimacyAdjustmentRateParameterName() {
    return kAdjustmentRateParameter;
}

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
