# Civilization Culture Diffusion Model

The installed `StonePotatoes::Civilization` target provides
`civilization/culture/CultureDiffusion.h` for monthly diffusion of caller-defined
cultural traits across regions. Trait values are normalized to `[0, 1]`; the
engine does not assign meanings such as language, religion, or values.

`initialTraitsByRegion` is a rectangular region-by-trait table. The directed
`monthlyAssimilationRateFromRegionToRegion` matrix uses source regions as rows
and destination regions as columns. Each value is the fraction of the
destination's difference from that source that is assimilated per step. Incoming
rates for each destination must sum to at most one, which keeps updates stable
and within the normalized range. Regions with no incoming contacts retain their
traits.

```cpp
using namespace StonePotatoes::Civilization::Culture;

CultureDiffusionConfig config;
config.initialTraitsByRegion = {
    {0.2f, 0.8f}, // region 0: caller-defined trait 0 and trait 1
    {0.8f, 0.2f}, // region 1
};
config.monthlyAssimilationRateFromRegionToRegion = {
    {0.0f, 0.25f}, // region 0 influences region 1
    {0.25f, 0.0f}, // region 1 influences region 0
};

auto culture = BuildCultureDiffusionModel(config);
culture.Step();
const float trait = culture.Stock(CultureTraitStockName(0, 0));
```

Trait stocks can be queried by region and trait index. Contact rates can be
changed between steps with
`CultureAssimilationParameterName(source, destination)`. Each step reads the
previous state synchronously, so region iteration order does not change the
result. The model represents diffusion only; it does not prescribe cultural
categories, create traits, or model political institutions.
