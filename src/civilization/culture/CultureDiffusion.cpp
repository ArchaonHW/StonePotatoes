// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "CultureDiffusion.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
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

std::vector<std::size_t> ClusterCultureRegionsBySimilarity(
    const std::vector<std::vector<float>>& traitsByRegion,
    float maxDistance) {
    if (!std::isfinite(maxDistance) || maxDistance < 0.0f) {
        throw std::invalid_argument(
            "culture cluster distance must be finite and nonnegative");
    }
    if (traitsByRegion.empty()) return {};
    const std::size_t traitCount = traitsByRegion.front().size();
    if (traitCount == 0) {
        throw std::invalid_argument(
            "culture clustering requires at least one trait");
    }
    for (const auto& traits : traitsByRegion) {
        if (traits.size() != traitCount) {
            throw std::invalid_argument(
                "culture clustering requires rectangular trait vectors");
        }
        for (float value : traits) {
            if (!std::isfinite(value) || value < 0.0f || value > 1.0f) {
                throw std::invalid_argument(
                    "culture cluster traits must be finite and in [0, 1]");
            }
        }
    }

    const std::size_t regionCount = traitsByRegion.size();
    std::vector<std::size_t> parent(regionCount);
    for (std::size_t i = 0; i < regionCount; ++i) parent[i] = i;
    const auto findRoot = [&](auto&& self, std::size_t region) -> std::size_t {
        if (parent[region] != region)
            parent[region] = self(self, parent[region]);
        return parent[region];
    };
    const double distanceSquared =
        static_cast<double>(maxDistance) * maxDistance;
    for (std::size_t a = 0; a < regionCount; ++a) {
        for (std::size_t b = a + 1; b < regionCount; ++b) {
            double separation = 0.0;
            for (std::size_t trait = 0; trait < traitCount; ++trait) {
                const double delta = static_cast<double>(
                    traitsByRegion[a][trait] - traitsByRegion[b][trait]);
                separation += delta * delta;
            }
            if (separation <= distanceSquared) {
                std::size_t rootA = findRoot(findRoot, a);
                std::size_t rootB = findRoot(findRoot, b);
                if (rootA != rootB) {
                    const std::size_t root = std::min(rootA, rootB);
                    parent[std::max(rootA, rootB)] = root;
                }
            }
        }
    }

    std::vector<std::size_t> groupByRoot(
        regionCount, std::numeric_limits<std::size_t>::max());
    std::vector<std::size_t> groups(regionCount);
    std::size_t nextGroup = 0;
    for (std::size_t region = 0; region < regionCount; ++region) {
        const std::size_t root = findRoot(findRoot, region);
        if (groupByRoot[root] == std::numeric_limits<std::size_t>::max())
            groupByRoot[root] = nextGroup++;
        groups[region] = groupByRoot[root];
    }
    return groups;
}

std::vector<std::size_t> CultureIdentityTracker::Update(
    const std::vector<std::vector<float>>& traitsByRegion,
    float maxDistance) {
    if (!m_PreviousIdentityByRegion.empty() &&
        traitsByRegion.size() != m_PreviousIdentityByRegion.size()) {
        throw std::invalid_argument(
            "culture identity tracking requires stable region count and order");
    }
    const std::vector<std::size_t> clusters =
        ClusterCultureRegionsBySimilarity(traitsByRegion, maxDistance);
    if (clusters.empty()) {
        m_PreviousIdentityByRegion.clear();
        return {};
    }

    const std::size_t clusterCount =
        *std::max_element(clusters.begin(), clusters.end()) + 1;
    std::map<std::size_t, std::map<std::size_t, std::size_t>> overlap;
    if (!m_PreviousIdentityByRegion.empty()) {
        for (std::size_t region = 0; region < clusters.size(); ++region) {
            ++overlap[m_PreviousIdentityByRegion[region]][clusters[region]];
        }
    }

    struct Match {
        std::size_t overlap = 0;
        std::size_t identity = 0;
        std::size_t cluster = 0;
    };
    std::vector<Match> matches;
    for (const auto& [identity, clustersById] : overlap) {
        for (const auto& [cluster, count] : clustersById) {
            matches.push_back({count, identity, cluster});
        }
    }
    std::sort(matches.begin(), matches.end(), [](const Match& lhs,
                                                 const Match& rhs) {
        if (lhs.overlap != rhs.overlap) return lhs.overlap > rhs.overlap;
        if (lhs.identity != rhs.identity) return lhs.identity < rhs.identity;
        return lhs.cluster < rhs.cluster;
    });

    std::vector<std::size_t> identityByCluster(clusterCount, 0);
    std::vector<bool> assignedCluster(clusterCount, false);
    std::set<std::size_t> assignedIdentity;
    for (const Match& match : matches) {
        if (assignedCluster[match.cluster] ||
            assignedIdentity.count(match.identity) != 0) {
            continue;
        }
        identityByCluster[match.cluster] = match.identity;
        assignedCluster[match.cluster] = true;
        assignedIdentity.insert(match.identity);
    }

    std::size_t nextIdentityId = m_NextIdentityId;
    for (std::size_t cluster = 0; cluster < clusterCount; ++cluster) {
        if (assignedCluster[cluster]) continue;
        if (nextIdentityId == std::numeric_limits<std::size_t>::max()) {
            throw std::overflow_error("culture identity id space exhausted");
        }
        identityByCluster[cluster] = nextIdentityId++;
    }
    std::vector<std::size_t> result;
    result.reserve(clusters.size());
    for (std::size_t cluster : clusters) {
        result.push_back(identityByCluster[cluster]);
    }
    m_PreviousIdentityByRegion = result;
    m_NextIdentityId = nextIdentityId;
    return result;
}

void CultureIdentityTracker::Reset() {
    m_PreviousIdentityByRegion.clear();
    m_NextIdentityId = 0;
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
