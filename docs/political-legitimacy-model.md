# Political legitimacy model

`StonePotatoes::Civilization::Politics::BuildPoliticalLegitimacyModel` provides
a bounded, deterministic monthly government-legitimacy stock. It is a small
scenario model: the game supplies normalized public-service satisfaction, tax
burden, and institutional cohesion values; the engine does not infer them from
economy or population simulation.

The target legitimacy is the weighted mean

```text
target = (serviceWeight * serviceSatisfaction
        + taxWeight * (1 - taxBurden)
        + cohesionWeight * institutionalCohesion) / totalWeight
```

Each monthly step moves the current value toward the target by
`adjustmentRatePerMonth * (target - current)`. The adjustment rate is limited
to `[0, 1]`, so the Euler step cannot overshoot; legitimacy remains in `[0, 1]`.
Weights must be finite, nonnegative, and have a positive sum. Runtime inputs
are exposed as model parameters and clamped to `[0, 1]` when evaluated.

```cpp
using namespace StonePotatoes::Civilization::Politics;

PoliticalLegitimacyConfig config;
config.initialLegitimacy = 0.4f;
config.publicServiceSatisfaction = 0.8f;
config.taxBurden = 0.3f;
config.institutionalCohesion = 0.6f;
config.adjustmentRatePerMonth = 0.15f;
auto legitimacy = BuildPoliticalLegitimacyModel(config);

legitimacy.SetParam(TaxBurdenParameterName(), 0.5f); // update scenario input
legitimacy.Step();
const float score = legitimacy.Stock(GovernmentLegitimacyStockName());
```

The model does not choose government types or claim that these three inputs
are universal measures of legitimacy. A separate
[`plurality-election-model.md`](plurality-election-model.md) tallies supplied
vote totals, but it does not collect ballots or simulate election procedures.
Government types, election rules, and leadership succession still need
explicit game-design rules and remain future work.
