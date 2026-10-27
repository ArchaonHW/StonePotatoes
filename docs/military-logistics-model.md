# Civilization Military Logistics Model

The installed `StonePotatoes::Civilization` target provides
`civilization/military/MilitaryLogistics.h` for a deterministic monthly troop
and supply stock model.

Each troop consumes a caller-defined amount of supplies per month. Supply
production is added before current-month demand is served. If the available
stock and production do not meet demand, the shortage fraction applies the
configured shortage attrition rate to troops. A separate base attrition rate
can model losses unrelated to supply. Recruitment and supply production are
external inputs and can be adjusted between steps with `SetParam`.

```cpp
using namespace StonePotatoes::Civilization::Military;

MilitaryLogisticsConfig config;
config.initialTroops = 100.0f;
config.initialSupplies = 50.0f;
config.supplyPerTroopPerMonth = 1.0f;
config.shortageAttritionRatePerMonth = 0.2f;

auto force = BuildMilitaryLogisticsModel(config);
force.Step(); // half the demand is unmet; shortage attrition removes 10 troops
const float troops = force.Stock(MilitaryTroopStockName());
```

Each step represents one month; retain the model's default `dt` of `1`.
This slice does not model battles, unit classes, equipment, movement, or
recruitment costs. The game should supply recruitment and production rates
from its economy and policy systems.
