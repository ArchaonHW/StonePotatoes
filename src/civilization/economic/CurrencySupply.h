// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/economic/SystemDynamics.h"

#include <limits>

namespace StonePotatoes {
namespace Civilization {
namespace Economic {

/** Configuration for an aggregate currency supply policy. */
struct CurrencySupplyConfig {
    float initialSupply = 0.0f;
    float monthlyIssuance = 0.0f;
    float monthlyBurn = 0.0f;
    float maximumSupply = std::numeric_limits<float>::max();
};

/** Build a monthly supply model with issuance capped by maximumSupply. */
SDModel BuildCurrencySupplyModel(const CurrencySupplyConfig& config);

const char* CurrencySupplyStockName();
const char* CurrencyIssuanceParameterName();
const char* CurrencyBurnParameterName();

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
