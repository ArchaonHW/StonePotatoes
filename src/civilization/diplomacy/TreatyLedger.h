// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include <cstdint>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Diplomacy {

enum class TreatyType : std::uint8_t {
    TradePact,
    NonAggressionPact,
    Alliance,
};

/** Active bilateral treaty; faction ids are stored in ascending order. */
struct Treaty {
    std::uint32_t factionA = 0;
    std::uint32_t factionB = 0;
    TreatyType type = TreatyType::TradePact;
    std::uint32_t monthsRemaining = 0;
};

/**
 * Deterministic, in-memory ledger for finite-duration bilateral treaties.
 * The caller decides which actions create treaties and how their effects apply.
 */
class TreatyLedger {
public:
    // Returns false if the same treaty is already active; it is not extended.
    bool Sign(TreatyType type, std::uint32_t factionA,
              std::uint32_t factionB, std::uint32_t durationMonths);
    bool Revoke(TreatyType type, std::uint32_t factionA,
                std::uint32_t factionB);
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
