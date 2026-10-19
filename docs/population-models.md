# Civilization Population Models

The installed `StonePotatoes::Civilization` target provides three population
building blocks in `civilization/population/PopulationDynamics.h`:

- `BuildPopulationModel`: monthly logistic growth with net migration.
- `BuildDemographicPopulationModel`: monthly density-limited births, deaths,
  and net migration.
- `BuildAgeStructuredPopulationModel`: annual cohort transitions, cohort-level
  survival, births, age-specific net migration, and a total carrying-capacity
  bound.

## Age-structured annual model

`AgeStructuredPopulationConfig` takes three equally sized vectors:

- `initialPopulationByAgeGroup`: starting population in each cohort.
- `annualSurvivalRateByAgeGroup`: fraction surviving the year, in `[0, 1]`.
- `annualBirthsPerPersonByAgeGroup`: births per person over one year; callers
  can include the reproductive share in this coefficient.
- `annualNetMigrationByAgeGroup`: optional signed net arrivals per year for
  each cohort. Positive values add residents; negative values remove them. An
  empty vector means zero migration in all age groups.
- `carryingCapacity`: maximum total population across all cohorts.

Each call to `SDModel::Step()` represents one year for this model. Each cohort
except the last sends its survivors into the next age group; the last group is
open-ended and surviving residents remain there. Births enter group zero and
are capped so projected survivors plus births do not exceed carrying capacity.
Rates can be changed between steps with `SetParam` using
`AgeGroupSurvivalParameterName(index)` and
`AgeGroupBirthRateParameterName(index)`. Migration can be changed using
`AgeGroupNetMigrationParameterName(index)`. Emigration is limited to the
projected population in that cohort after aging and births. Immigration is
proportionally scaled across cohorts when projected survivors and births, after
capped outflows, leave less room than requested under the total carrying
capacity. Read cohort stocks using
`AgeGroupStockName(index)`. Do not change the returned model's `dt`: cohort
advancement and the supplied rates are annual.

```cpp
using namespace StonePotatoes::Civilization::Population;

AgeStructuredPopulationConfig config;
config.initialPopulationByAgeGroup = {1000.0f, 800.0f, 200.0f};
config.annualSurvivalRateByAgeGroup = {0.995f, 0.98f, 0.75f};
config.annualBirthsPerPersonByAgeGroup = {0.0f, 0.04f, 0.0f};
config.carryingCapacity = 5000.0f;

auto population = BuildAgeStructuredPopulationModel(config);
population.Step(); // advance one year
const float youngCohort = population.Stock(AgeGroupStockName(0));
```

The model is a deterministic, single-region demographic slice. It does not
currently model sex-separated fertility, multi-region flows, or calibration
from census data. Use the other monthly model when
scenario rates are expressed per month.
