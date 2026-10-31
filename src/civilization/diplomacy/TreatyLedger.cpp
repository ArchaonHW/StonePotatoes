// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "civilization/diplomacy/TreatyLedger.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <tuple>

namespace StonePotatoes {
namespace Civilization {
namespace Diplomacy {
namespace {

void ValidateTreaty(TreatyType type, std::uint32_t factionA,
                    std::uint32_t factionB) {
    if (type != TreatyType::TradePact &&
        type != TreatyType::NonAggressionPact && type != TreatyType::Alliance) {
        throw std::invalid_argument("unknown treaty type");
    }
    if (factionA == factionB) {
        throw std::invalid_argument("a treaty requires two distinct factions");
    }
}

std::pair<std::uint32_t, std::uint32_t> CanonicalPair(
    std::uint32_t factionA, std::uint32_t factionB) {
    return {std::min(factionA, factionB), std::max(factionA, factionB)};
}

void ValidateTerms(const TreatyTerms& terms) {
    const float values[] = {
        terms.monthlyPaymentFromAToB,
        terms.monthlyPaymentFromBToA,
        terms.breachPenaltyWhenAIsBreacher,
        terms.breachPenaltyWhenBIsBreacher};
    for (float value : values) {
        if (!std::isfinite(value) || value < 0.0f) {
            throw std::invalid_argument(
                "treaty payments and breach penalties must be finite and "
                "nonnegative");
        }
    }
}

} // namespace

bool TreatyLedger::Sign(TreatyType type, std::uint32_t factionA,
                        std::uint32_t factionB,
                        std::uint32_t durationMonths,
                        const TreatyTerms& terms) {
    ValidateTreaty(type, factionA, factionB);
    ValidateTerms(terms);
    if (durationMonths == 0) {
        throw std::invalid_argument("treaty duration must be positive");
    }
    const auto [canonicalA, canonicalB] = CanonicalPair(factionA, factionB);
    if (HasTreaty(type, canonicalA, canonicalB)) return false;

    TreatyTerms canonicalTerms = terms;
    if (factionA != canonicalA) {
        std::swap(canonicalTerms.monthlyPaymentFromAToB,
                  canonicalTerms.monthlyPaymentFromBToA);
        std::swap(canonicalTerms.breachPenaltyWhenAIsBreacher,
                  canonicalTerms.breachPenaltyWhenBIsBreacher);
    }

    m_Treaties.push_back({canonicalA, canonicalB, type, durationMonths,
                          canonicalTerms});
    std::sort(m_Treaties.begin(), m_Treaties.end(),
              [](const Treaty& lhs, const Treaty& rhs) {
                  return std::tie(lhs.type, lhs.factionA, lhs.factionB) <
                         std::tie(rhs.type, rhs.factionA, rhs.factionB);
              });
    return true;
}

std::vector<TreatyPayment> TreatyLedger::MonthlyObligations() const {
    std::vector<TreatyPayment> obligations;
    for (const Treaty& treaty : m_Treaties) {
        if (treaty.terms.monthlyPaymentFromAToB > 0.0f) {
            obligations.push_back({treaty.type, treaty.factionA,
                                   treaty.factionB,
                                   treaty.terms.monthlyPaymentFromAToB});
        }
        if (treaty.terms.monthlyPaymentFromBToA > 0.0f) {
            obligations.push_back({treaty.type, treaty.factionB,
                                   treaty.factionA,
                                   treaty.terms.monthlyPaymentFromBToA});
        }
    }
    return obligations;
}

std::optional<TreatyBreach> TreatyLedger::Breach(
    TreatyType type, std::uint32_t breachingFaction,
    std::uint32_t counterpartyFaction) {
    ValidateTreaty(type, breachingFaction, counterpartyFaction);
    const auto [canonicalA, canonicalB] =
        CanonicalPair(breachingFaction, counterpartyFaction);
    const auto it = std::find_if(m_Treaties.begin(), m_Treaties.end(),
        [&](const Treaty& treaty) {
            return treaty.type == type && treaty.factionA == canonicalA &&
                   treaty.factionB == canonicalB;
        });
    if (it == m_Treaties.end()) return std::nullopt;

    const bool breacherIsA = breachingFaction == canonicalA;
    TreatyBreach result{type, breachingFaction, counterpartyFaction,
        breacherIsA ? it->terms.breachPenaltyWhenAIsBreacher
                    : it->terms.breachPenaltyWhenBIsBreacher};
    m_Treaties.erase(it);
    return result;
}

bool TreatyLedger::Revoke(TreatyType type, std::uint32_t factionA,
                          std::uint32_t factionB) {
    ValidateTreaty(type, factionA, factionB);
    const auto [canonicalA, canonicalB] = CanonicalPair(factionA, factionB);
    const auto oldSize = m_Treaties.size();
    std::erase_if(m_Treaties, [&](const Treaty& treaty) {
        return treaty.type == type && treaty.factionA == canonicalA &&
               treaty.factionB == canonicalB;
    });
    return m_Treaties.size() != oldSize;
}

bool TreatyLedger::HasTreaty(TreatyType type, std::uint32_t factionA,
                             std::uint32_t factionB) const {
    ValidateTreaty(type, factionA, factionB);
    const auto [canonicalA, canonicalB] = CanonicalPair(factionA, factionB);
    return std::any_of(m_Treaties.begin(), m_Treaties.end(),
                       [&](const Treaty& treaty) {
                           return treaty.type == type &&
                                  treaty.factionA == canonicalA &&
                                  treaty.factionB == canonicalB;
                       });
}

bool TreatyLedger::CanDeclareWar(std::uint32_t attackerFaction,
                                 std::uint32_t targetFaction) const {
    ValidateTreaty(TreatyType::NonAggressionPact, attackerFaction,
                   targetFaction);
    return !HasTreaty(TreatyType::NonAggressionPact, attackerFaction,
                      targetFaction) &&
           !HasTreaty(TreatyType::Alliance, attackerFaction, targetFaction);
}

void TreatyLedger::AdvanceMonth() {
    for (Treaty& treaty : m_Treaties) {
        if (treaty.monthsRemaining > 0) --treaty.monthsRemaining;
    }
    std::erase_if(m_Treaties, [](const Treaty& treaty) {
        return treaty.monthsRemaining == 0;
    });
}

} // namespace Diplomacy
} // namespace Civilization
} // namespace StonePotatoes
