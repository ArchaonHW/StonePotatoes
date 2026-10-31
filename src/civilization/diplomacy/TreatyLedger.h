// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Diplomacy {

enum class TreatyType : std::uint8_t {
    TradePact,
    NonAggressionPact,
    Alliance,
};

/** Bilateral monthly payments and one-time breach penalties. */
struct TreatyTerms {
    // Directional fields refer to the factionA/factionB arguments to Sign.
    float monthlyPaymentFromAToB = 0.0f;
    float monthlyPaymentFromBToA = 0.0f;
    float breachPenaltyWhenAIsBreacher = 0.0f;
    float breachPenaltyWhenBIsBreacher = 0.0f;
};

/** Active bilateral treaty; faction ids are stored in ascending order. */
struct Treaty {
    std::uint32_t factionA = 0;
    std::uint32_t factionB = 0;
    TreatyType type = TreatyType::TradePact;
    std::uint32_t monthsRemaining = 0;
    // Terms are normalized to the canonical stored factionA/factionB order.
    TreatyTerms terms;
};

struct TreatyPayment {
    TreatyType type = TreatyType::TradePact;
    std::uint32_t payerFaction = 0;
    std::uint32_t receiverFaction = 0;
    float amount = 0.0f;
};

struct TreatyBreach {
    TreatyType type = TreatyType::TradePact;
    std::uint32_t breachingFaction = 0;
    std::uint32_t aggrievedFaction = 0;
    float penaltyDue = 0.0f;
};

/**
 * Deterministic, in-memory ledger for finite-duration bilateral treaties.
 * The caller decides which actions create treaties and how their effects apply.
 */
class TreatyLedger {
public:
    // Returns false if the same treaty is already active; it is not extended.
    bool Sign(TreatyType type, std::uint32_t factionA,
              std::uint32_t factionB, std::uint32_t durationMonths,
              const TreatyTerms& terms = {});
    bool Revoke(TreatyType type, std::uint32_t factionA,
                std::uint32_t factionB);
    // Return this month's obligations in deterministic treaty order. Call
    // before AdvanceMonth(); the caller applies payments to its own economy.
    std::vector<TreatyPayment> MonthlyObligations() const;
    // Break an active treaty and report its one-time caller-defined penalty.
    // No result means the treaty was not active; no treasury is mutated here.
    std::optional<TreatyBreach> Breach(TreatyType type,
                                       std::uint32_t breachingFaction,
                                       std::uint32_t counterpartyFaction);
    bool HasTreaty(TreatyType type, std::uint32_t factionA,
                   std::uint32_t factionB) const;
    // Non-aggression pacts and alliances prohibit declaring war while active.
    bool CanDeclareWar(std::uint32_t attackerFaction,
                       std::uint32_t targetFaction) const;
    // Advance one month and remove treaties whose term reaches zero.
    void AdvanceMonth();
    const std::vector<Treaty>& ActiveTreaties() const { return m_Treaties; }

private:
    std::vector<Treaty> m_Treaties;
};

} // namespace Diplomacy
} // namespace Civilization
} // namespace StonePotatoes
