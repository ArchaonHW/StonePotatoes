# Civilization Battle Resolution Model

The installed `StonePotatoes::Civilization` target provides
`civilization/military/BattleResolution.h` for resolving one simultaneous
volley between two aggregated forces.

For each side, incoming damage is the opposing unit count multiplied by that
side's damage per unit. Casualties equal incoming damage divided by the target
side's durability, capped at the number of units present. Both volleys use the
initial force sizes, so either or both sides can be eliminated in the same
exchange. Counts and losses are floats, allowing expected-value simulation
without random sampling.

```cpp
using namespace StonePotatoes::Civilization::Military;

EngagementConfig config;
config.attackerUnits = 100.0f;
config.defenderUnits = 80.0f;
config.attackerDamagePerUnit = 2.0f;
config.defenderDamagePerUnit = 1.0f;
config.attackerDurability = 2.0f;
config.defenderDurability = 4.0f;

const EngagementResult result = ResolveEngagement(config);
// 40 attacker losses and 50 defender losses; both sides fire simultaneously.
```

All inputs must be finite and nonnegative, except durability, which must be
strictly positive. This one-volley abstraction does not model terrain, morale,
initiative, unit types, randomness, or multi-round tactics. Callers can compose
it with the military logistics model and their own battle rules.
