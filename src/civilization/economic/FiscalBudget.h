// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/economic/SystemDynamics.h"

namespace StonePotatoes {
namespace Civilization {
namespace Economic {

/** Configuration for a monthly public treasury budget. */
struct FiscalBudgetConfig {
    float initialTreasury = 0.0f;
    // Monthly taxable output is an external aggregate supplied by the game.
    float monthlyTaxBase = 0.0f;
    float taxRate = 0.0f;
    float monthlyPublicSpending = 0.0f;
};

/** Build a monthly budget whose spending cannot exceed available treasury. */
SDModel BuildFiscalBudgetModel(const FiscalBudgetConfig& config);

const char* FiscalTreasuryStockName();
const char* FiscalTaxBaseParameterName();
const char* FiscalTaxRateParameterName();
const char* FiscalPublicSpendingParameterName();

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
