// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "CultureFormation.h"

#include "CultureDiffusion.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Culture {

std::string CultureInnovationRateParameterName(std::size_t regionIndex,
                                               std::size_t traitIndex) {
    return "culture_innovation_rate_region_" + std::to_string(regionIndex) +
           "_trait_" + std::to_string(traitIndex);
}

std::string CultureDecayRateParameterName(std::size_t regionIndex,
                                          std::size_t traitIndex) {
    return "culture_decay_rate_region_" + std::to_string(regionIndex) +
           "_trait_" + std::to_string(traitIndex);
}

Economic::SDModel BuildCultureFormationModel(
    const CultureFormationConfig& config) {
    const std::size_t regionCount = config.initialTraitsByRegion.size();
    if (regionCount == 0 || config.initialTraitsByRegion.front().empty() ||
        config.monthlyInnovationRateByRegion.size() != regionCount ||
        config.monthlyDecayRateByRegion.size() != regionCount) {
        throw std::invalid_argument(
            "culture formation requires matching nonempty region tables");
    }

    const std::size_t traitCount = config.initialTraitsByRegion.front().size();
    for (std::size_t region = 0; region < regionCount; ++region) {
        if (config.initialTraitsByRegion[region].size() != traitCount ||
            config.monthlyInnovationRateByRegion[region].size() != traitCount ||
            config.monthlyDecayRateByRegion[region].size() != traitCount) {
            throw std::invalid_argument(
                "culture formation tables must have matching rectangular rows");
        }
        for (std::size_t trait = 0; trait < traitCount; ++trait) {
            const float initial = config.initialTraitsByRegion[region][trait];
            const float innovation =
                config.monthlyInnovationRateByRegion[region][trait];
            const float decay = config.monthlyDecayRateByRegion[region][trait];
            if (!std::isfinite(initial) || initial < 0.0f || initial > 1.0f) {
                throw std::invalid_argument(
                    "culture formation traits must be finite and in [0, 1]");
            }
            if (!std::isfinite(innovation) || innovation < 0.0f ||
                !std::isfinite(decay) || decay < 0.0f ||
                innovation + decay > 1.0f) {
                throw std::invalid_argument(
                    "culture innovation and decay rates must be finite, "
                    "nonnegative, and sum to at most 1");
            }
        }
    }

    Economic::SDModel model;
    for (std::size_t region = 0; region < regionCount; ++region) {
        for (std::size_t trait = 0; trait < traitCount; ++trait) {
            const std::string stock = CultureTraitStockName(region, trait);
            const std::string innovationName =
                CultureInnovationRateParameterName(region, trait);
            const std::string decayName =
                CultureDecayRateParameterName(region, trait);
            model.AddStock(stock, config.initialTraitsByRegion[region][trait],
                           0.0f, 1.0f);
            model.SetParam(innovationName,
                config.monthlyInnovationRateByRegion[region][trait]);
            model.SetParam(decayName,
                config.monthlyDecayRateByRegion[region][trait]);
            model.AddFlow(stock,
                [stock, innovationName, decayName](
                    const Economic::SDState& state) {
                    const float value = std::clamp(state.S(stock), 0.0f, 1.0f);
                    const float rawInnovation = state.P(innovationName);
                    const float rawDecay = state.P(decayName);
                    const float innovation = std::isfinite(rawInnovation)
                        ? std::clamp(rawInnovation, 0.0f, 1.0f) : 0.0f;
                    const float decay = std::isfinite(rawDecay)
                        ? std::clamp(rawDecay, 0.0f, 1.0f) : 0.0f;
                    const float totalRate = innovation + decay;
                    const float scale = totalRate > 1.0f ? 1.0f / totalRate
                                                         : 1.0f;
                    return scale * (innovation * (1.0f - value) -
                                    decay * value);
                });
        }
    }
    return model;
}

} // namespace Culture
} // namespace Civilization
} // namespace StonePotatoes
