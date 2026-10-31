# Civilization Diplomacy Relations Model

The installed `StonePotatoes::Civilization` target provides
`civilization/diplomacy/Diplomacy.h` for a deterministic monthly model of
directed faction relations.

Relation scores range from `-1` (hostile) to `1` (friendly). The matrix is
indexed `[observer][target]`, so two factions may hold different views of each
other. The caller supplies a signed monthly change for each pair; the model
applies the change and clamps the score to its supported range. Parameters can
be adjusted between steps with `MonthlyRelationChangeParameterName`.

```cpp
using namespace StonePotatoes::Civilization::Diplomacy;

DiplomacyConfig config;
config.initialRelationByFaction = {{0.0f, 0.4f}, {-0.2f, 0.0f}};
config.monthlyRelationChangeByFaction = {{0.0f, 0.1f}, {0.0f, 0.0f}};

auto relations = BuildDiplomacyModel(config);
relations.Step(); // faction 0's view of faction 1 rises to 0.5
const float view = relations.Stock(RelationStockName(0, 1));
```

The model does not decide what actions improve relations, negotiate treaties,
or declare war. Game rules should calculate those effects and provide the
monthly changes. Each step represents one month; retain the model's default
`dt` of `1`.

## Finite-duration treaties

`TreatyLedger` stores bilateral trade pacts, non-aggression pacts, and
alliances. Faction order is symmetric: signing a pact as `(A, B)` makes it
visible as `(B, A)`. Duplicate active treaties are rejected without extending
their term. `AdvanceMonth()` decrements every active term and removes expired
entries. The ledger is in-memory; the game remains responsible for saving it.

`CanDeclareWar(A, B)` returns false while the pair has an active
non-aggression pact or alliance. Trade pacts do not restrict war declarations.
The battle and game-action code should check this before starting an engagement.

```cpp
TreatyLedger treaties;
treaties.Sign(TreatyType::NonAggressionPact, 2, 7, 12);
if (treaties.CanDeclareWar(7, 2)) {
    // Start the war and resolve the engagement.
}
treaties.AdvanceMonth();
```

## Treaty obligations and breach penalties

`TreatyTerms` optionally adds directional monthly payments and one-time
breach penalties. Amounts are finite, nonnegative, caller-defined units; the
ledger does not debit treasuries or select a currency. Directional fields refer
to the faction order passed to `Sign`, even though the stored treaty pair is
normalized. For each active treaty, `MonthlyObligations()` returns positive
payments in deterministic treaty order. Apply these obligations in the game
economy before calling `AdvanceMonth()` for that period.

`Breach(type, breachingFaction, counterpartyFaction)` removes an active treaty
and returns the breacher, aggrieved faction, and applicable one-time penalty.
It returns no value when no matching treaty is active. The caller decides how
to collect the penalty and apply relation, reputation, or other consequences.
`Revoke()` remains a penalty-free administrative removal.

```cpp
TreatyLedger contracts;
TreatyTerms terms;
terms.monthlyPaymentFromAToB = 5.0f;
terms.breachPenaltyWhenAIsBreacher = 30.0f;
contracts.Sign(TreatyType::TradePact, 2, 7, 12, terms);

for (const TreatyPayment& payment : contracts.MonthlyObligations()) {
    // Settle payment.payerFaction -> payment.receiverFaction in game economy.
}
if (const auto breach = contracts.Breach(TreatyType::TradePact, 2, 7)) {
    // Apply breach->penaltyDue to the aggrieved faction or game rules.
}
```
