// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "CultureDiffusion.h"

#include <cmath>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Culture {

std::string CultureTraitStockName(std::size_t regionIndex,
                                  std::size_t traitIndex) {
    return "culture_trait_region_" + std::to_string(regionIndex) +
           "_trait_" + std::to_string(traitIndex);
}

std::string CultureAssimilationParameterName(std::size_t fromRegionIndex,
                                             std::size_t toRegionIndex) {
    return "culture_assimilation_rate_region_" +
           std::to_string(fromRegionIndex) + "_to_" +
           std::to_string(toRegionIndex);
}

Economic::SDModel BuildCultureDiffusionModel(
    const CultureDiffusionConfig& config) {
    const std::size_t regionCount = config.initialTraitsByRegion.size();
    if (regionCount == 0 || config.initialTraitsByRegion.front().empty() ||
        config.monthlyAssimilationRateFromRegionToRegion.size() !=
            regionCount) {
        throw std::invalid_argument(
            "culture regions, traits, and contact matrix must be nonempty "
            "and match");
    }

    const std::size_t traitCount = config.initialTraitsByRegion.front().size();
    std::vector<double> incomingRates(regionCount, 0.0);
    for (std::size_t region = 0; region < regionCount; ++region) {
        if (config.initialTraitsByRegion[region].size() != traitCount ||
            config.monthlyAssimilationRateFromRegionToRegion[region].size() !=
                regionCount) {
            throw std::invalid_argument(
                "culture trait rows and contact matrix must be rectangular "
                "and square");
        }
        for (float value : config.initialTraitsByRegion[region]) {
            if (!std::isfinite(value) || value < 0.0f || value > 1.0f) {
                throw std::invalid_argument(
                    "culture trait values must be finite and in [0, 1]");
            }
        }
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            const float rate =
                config.monthlyAssimilationRateFromRegionToRegion[region]
                    [destination];
            if (!std::isfinite(rate) || rate < 0.0f || rate > 1.0f ||
                (region == destination && rate != 0.0f)) {
                throw std::invalid_argument(
                    "culture assimilation rates must be finite in [0, 1] "
                    "and exclude self-links");
            }
            incomingRates[destination] += rate;
        }
    }
    for (double rate : incomingRates) {
        if (rate > 1.0) {
            throw std::invalid_argument(
                "incoming culture assimilation rates must sum to at most 1");
        }
    }

    Economic::SDModel model;
    std::vector<std::vector<std::string>> traitStockNames(
        regionCount, std::vector<std::string>(traitCount));
    std::vector<std::vector<std::string>> assimilationParameterNames(
        regionCount, std::vector<std::string>(regionCount));
    for (std::size_t region = 0; region < regionCount; ++region) {
        for (std::size_t trait = 0; trait < traitCount; ++trait) {
            const std::string stockName =
                CultureTraitStockName(region, trait);
            traitStockNames[region][trait] = stockName;
            model.AddStock(stockName,
                           config.initialTraitsByRegion[region][trait],
                           0.0f, 1.0f);
        }
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            const std::string parameterName =
                CultureAssimilationParameterName(region, destination);
            assimilationParameterNames[region][destination] = parameterName;
            model.SetParam(parameterName,
                config.monthlyAssimilationRateFromRegionToRegion[region]
                    [destination]);
        }
    }

    for (std::size_t destination = 0; destination < regionCount;
         ++destination) {
        for (std::size_t trait = 0; trait < traitCount; ++trait) {
            const std::string destinationStock =
                traitStockNames[destination][trait];
            model.AddFlow(destinationStock,
                [destination, trait, regionCount, traitStockNames,
                 assimilationParameterNames](
                    const Economic::SDState& state) {
                    const float destinationValue =
                        state.S(traitStockNames[destination][trait]);
                    float change = 0.0f;
                    float totalRate = 0.0f;
                    for (std::size_t source = 0; source < regionCount;
                         ++source) {
                        if (source == destination) {
                            continue;
                        }
                        const float rawRate = state.P(
                            assimilationParameterNames[source][destination]);
                        if (!std::isfinite(rawRate)) {
                            continue;
                        }
                        const float rate = rawRate < 0.0f ? 0.0f
                            : rawRate > 1.0f ? 1.0f : rawRate;
                        totalRate += rate;
                        change += rate *
                            (state.S(traitStockNames[source][trait]) -
                             destinationValue);
                    }
                    return totalRate > 1.0f ? change / totalRate : change;
                });
        }
    }
    return model;
}

} // namespace Culture
} // namespace Civilization
} // namespace StonePotatoes
