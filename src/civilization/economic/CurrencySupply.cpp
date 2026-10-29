// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "civilization/economic/CurrencySupply.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Economic {

const char* CurrencySupplyStockName() { return "currency_supply"; }
const char* CurrencyIssuanceParameterName() {
    return "monthly_currency_issuance";
}
const char* CurrencyBurnParameterName() { return "monthly_currency_burn"; }

SDModel BuildCurrencySupplyModel(const CurrencySupplyConfig& config) {
    if (!std::isfinite(config.initialSupply) || config.initialSupply < 0.0f ||
        !std::isfinite(config.monthlyIssuance) ||
        config.monthlyIssuance < 0.0f ||
        !std::isfinite(config.monthlyBurn) || config.monthlyBurn < 0.0f ||
        !std::isfinite(config.maximumSupply) ||
        config.maximumSupply <= 0.0f ||
        config.initialSupply > config.maximumSupply) {
        throw std::invalid_argument("invalid currency supply configuration");
    }

    SDModel model;
    model.AddStock(CurrencySupplyStockName(), config.initialSupply, 0.0f,
                   config.maximumSupply);
    model.SetParam(CurrencyIssuanceParameterName(), config.monthlyIssuance);
    model.SetParam(CurrencyBurnParameterName(), config.monthlyBurn);
    model.AddFlow(CurrencySupplyStockName(), [cap = config.maximumSupply](
                                                const SDState& state) {
        const float stock = state.S(CurrencySupplyStockName());
        const float rawIssuance = state.P(CurrencyIssuanceParameterName());
        const float rawBurn = state.P(CurrencyBurnParameterName());
        const double supply = std::clamp(
            static_cast<double>(stock), 0.0, static_cast<double>(cap));
        const double issuance = std::isfinite(rawIssuance)
            ? std::max(0.0, static_cast<double>(rawIssuance)) : 0.0;
        const double burn = std::isfinite(rawBurn)
            ? std::max(0.0, static_cast<double>(rawBurn)) : 0.0;
        const double issued = std::min(issuance,
            std::max(0.0, static_cast<double>(cap) - supply));
        const double afterIssuance = supply + issued;
        const double burned = std::min(burn, afterIssuance);
        return static_cast<float>(issued - burned);
    });
    return model;
}

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
