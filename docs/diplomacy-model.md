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
