// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "civilization/economic/FiscalBudget.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Economic {

const char* FiscalTreasuryStockName() { return "public_treasury"; }
const char* FiscalTaxBaseParameterName() { return "monthly_tax_base"; }
const char* FiscalTaxRateParameterName() { return "tax_rate"; }
const char* FiscalPublicSpendingParameterName() {
    return "monthly_public_spending";
}

SDModel BuildFiscalBudgetModel(const FiscalBudgetConfig& config) {
    if (!std::isfinite(config.initialTreasury) ||
        config.initialTreasury < 0.0f ||
        !std::isfinite(config.monthlyTaxBase) || config.monthlyTaxBase < 0.0f ||
        !std::isfinite(config.taxRate) || config.taxRate < 0.0f ||
        config.taxRate > 1.0f ||
        !std::isfinite(config.monthlyPublicSpending) ||
        config.monthlyPublicSpending < 0.0f) {
        throw std::invalid_argument("invalid fiscal budget configuration");
    }

    SDModel model;
    model.AddStock(FiscalTreasuryStockName(), config.initialTreasury, 0.0f,
                   std::numeric_limits<float>::max());
    model.SetParam(FiscalTaxBaseParameterName(), config.monthlyTaxBase);
    model.SetParam(FiscalTaxRateParameterName(), config.taxRate);
    model.SetParam(FiscalPublicSpendingParameterName(),
                   config.monthlyPublicSpending);
    model.AddFlow(FiscalTreasuryStockName(), [](const SDState& state) {
        const float taxBase = state.P(FiscalTaxBaseParameterName());
        const float taxRate = state.P(FiscalTaxRateParameterName());
        const float requestedSpending =
            state.P(FiscalPublicSpendingParameterName());
        if (!std::isfinite(taxBase) || !std::isfinite(taxRate) ||
            !std::isfinite(requestedSpending)) {
            return 0.0f;
        }

        // Keep runtime scenario changes within the same invariants as config.
        const double revenue = static_cast<double>(std::max(0.0f, taxBase)) *
                               std::clamp(taxRate, 0.0f, 1.0f);
        const double available =
            static_cast<double>(std::max(
                0.0f, state.S(FiscalTreasuryStockName()))) + revenue;
        const double spending = std::min(
            static_cast<double>(std::max(0.0f, requestedSpending)), available);
        return static_cast<float>(revenue - spending);
    });
    return model;
}

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
