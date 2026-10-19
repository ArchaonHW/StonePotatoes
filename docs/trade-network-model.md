# Civilization Trade Network Model

The installed `StonePotatoes::Civilization` target provides
`civilization/economic/TradeNetwork.h` for a deterministic, monthly model of
one stored commodity across multiple regions.

`TradeNetworkConfig` requires equally sized vectors for initial stock, storage
capacity, monthly production, and monthly consumption. Its square
`monthlyTradeFromRegionToRegion` matrix uses rows as source regions and columns
as destinations. Each value is the requested quantity transferred per month;
self-routes must be zero.

At each step, local production is added and consumption is bounded by the
available local stock. The resulting inventory is capped to local storage
capacity. Routes then transfer inventory: outgoing requests are proportionally
reduced if a source lacks inventory, and incoming requests are proportionally
reduced if a destination lacks storage space. Each accepted quantity is
removed from its source and added to its destination, conserving trade flows.
Transfers happen after local consumption, so imported inventory is available
to local consumption from the next step onward. Production beyond storage
capacity is discarded.

Production, consumption, and route quantities can be changed between steps
with `TradeProductionParameterName(index)`,
`TradeConsumptionParameterName(index)`, and
`TradeRouteParameterName(from, to)`. Query inventory using
`TradeStockName(index)`.

```cpp
using namespace StonePotatoes::Civilization::Economic;

TradeNetworkConfig config;
config.initialStockByRegion = {100.0f, 20.0f};
config.storageCapacityByRegion = {500.0f, 500.0f};
config.monthlyProductionByRegion = {10.0f, 0.0f};
config.monthlyConsumptionByRegion = {5.0f, 0.0f};
config.monthlyTradeFromRegionToRegion = {
    {0.0f, 30.0f}, // region 0 sends up to 30 units to region 1
    {0.0f, 0.0f},
};

auto trade = BuildTradeNetworkModel(config);
trade.Step(); // advance one month
```

The model covers inventory and route quantities only; it does not simulate
prices, transport costs, taxes, or route delays.
