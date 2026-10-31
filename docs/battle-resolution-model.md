# Civilization Battle Resolution Model

The installed `StonePotatoes::Civilization` target provides
`civilization/military/BattleResolution.h` for resolving simultaneous volleys
between two aggregated forces.

For each side, incoming damage is the opposing unit count multiplied by that
side's damage per unit. Casualties equal incoming damage divided by the target
side's durability, capped at the number of units present. Both volleys use the
initial force sizes, so either or both sides can be eliminated in the same
exchange. Counts and losses are floats, allowing expected-value simulation
without random sampling.

`ResolveBattle(config, maxRounds)` repeats this volley with surviving force
counts, stopping when a force is eliminated, neither side takes losses, or the
caller-supplied round limit is reached. The maximum supported limit is
`MaxBattleRounds` (10,000). Each round reuses the same damage, durability,
morale, and terrain values; this models attrition over repeated volleys, not
changing tactical conditions.

```cpp
using namespace StonePotatoes::Civilization::Military;

EngagementConfig config;
config.attackerUnits = 100.0f;
config.defenderUnits = 80.0f;
config.attackerDamagePerUnit = 2.0f;
config.defenderDamagePerUnit = 1.0f;
config.attackerDurability = 2.0f;
config.defenderDurability = 4.0f;
config.attackerMoraleFactor = 0.8f;
config.defenderTerrainDamageReduction = 0.25f;

const EngagementResult result = ResolveEngagement(config);
// 40 attacker losses and 30 defender losses; both sides fire simultaneously.
const BattleResult battle = ResolveBattle(config, 4); // up to four volleys
```

Unit counts and damage values must be finite and nonnegative; durability must
be finite and strictly positive. Morale factors and terrain damage reductions
must be finite and in `[0, 1]`.
Morale multiplies damage dealt by that side. Terrain reduction decreases damage
received by that side. Defaults of morale `1` and terrain reduction `0` preserve
the original calculations. The model does not include initiative, unit types,
or randomness; callers can compose it with the military logistics model and
their own battle rules.
