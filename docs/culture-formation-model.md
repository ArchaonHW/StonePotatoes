# Civilization Culture Trait Formation

`StonePotatoes::Civilization` provides `civilization/culture/CultureFormation.h`
for deterministic, monthly formation and fading of caller-defined normalized
cultural traits. It does not assign meaning to a trait index; games can map
indices to their own concepts and supply their own per-region rates.

For trait value `x` in `[0, 1]`, innovation rate `i` and decay rate `d`, a step
applies:

```text
x(next) = x + i * (1 - x) - d * x
```

Rates are fractions per model step. Initial rates must be finite, nonnegative,
and sum to at most one, making the update a convex combination that stays
bounded. Innovation adds a fraction of remaining headroom; decay removes a
fraction of the current value. A zero rate freezes that process. Parameters
can be changed between steps using `CultureInnovationRateParameterName` and
`CultureDecayRateParameterName`; runtime values are clamped and normalized if
their sum exceeds one. Nonfinite runtime values are treated as zero.

```cpp
using namespace StonePotatoes::Civilization::Culture;

CultureFormationConfig config;
config.initialTraitsByRegion = {{0.2f, 0.8f}};
config.monthlyInnovationRateByRegion = {{0.5f, 0.0f}};
config.monthlyDecayRateByRegion = {{0.0f, 0.25f}};
auto culture = BuildCultureFormationModel(config);
culture.Step();
const float adopted = culture.Stock(CultureTraitStockName(0, 0));
```

This model represents local adoption and fading only. Use
`BuildCultureDiffusionModel` separately when regions exchange cultural traits;
the two models can be composed by transferring stocks at explicit simulation
boundaries.
