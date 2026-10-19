# Civilization Market Price Model

The installed `StonePotatoes::Civilization` target provides
`civilization/economic/MarketPrice.h` for a deterministic monthly price
adjustment driven by the current stock of one commodity.

`BuildMarketPriceModel` returns an `SDModel` with a `market_price` stock. Set
`market_stock` before each step to the current inventory (for example, from
`TradeStockName(region)`), and adjust `market_target_stock` or
`market_response_per_month` when running a scenario. The stock-to-target ratio
is clamped to a shortage/surplus signal in `[-1, 1]`; price rises below target,
falls above target, and holds at target. The monthly fractional adjustment is
`responsePerMonth * clamp(1 - stock / targetStock, -1, 1)`. Prices remain
within the configured minimum and maximum bounds.

The response is linear and intentionally small in scope. It provides a stable
price signal for simulation and strategy decisions; it does not model currency,
transaction settlement, inflation, taxes, or transport costs.

```cpp
using namespace StonePotatoes::Civilization::Economic;

MarketPriceConfig config;
config.initialPrice = 10.0f;
config.targetStock = 100.0f;
config.responsePerMonth = 0.2f;
config.minimumPrice = 1.0f;
config.maximumPrice = 100.0f;

auto price = BuildMarketPriceModel(config);
price.SetParam(MarketStockParameterName(), 50.0f);
price.Step(); // price rises to 11.0
```
