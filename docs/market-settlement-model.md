# Multi-commodity Market Settlement

The installed `StonePotatoes::Civilization` target exposes
`civilization/economic/MarketSettlement.h` for atomic settlement of proposed
trades across several commodities.

Each `MarketAccount` contains a currency balance and a map of commodity
inventories. A batch of `CommodityTradeOrder` values names the seller, buyer,
commodity, requested quantity, and unit price. `SettleCommodityTrades` returns
updated account copies and one accepted quantity/payment result per order; the
input accounts are never mutated.

Settlement uses opening balances only. For each seller and commodity, requested
outgoing quantities are proportionally reduced to available inventory. Then,
for each buyer, the cost of all incoming orders across every commodity is
proportionally reduced to available funds. This makes item and money limits
independent of the order list. Sellers receive payments after the batch, so
same-batch sale proceeds cannot fund purchases. Commodity quantities move
between accounts and currency is transferred between buyer and seller, with
neither created by settlement. Account inventories have no capacity limit in
this primitive; simulation callers should enforce storage capacity before
submitting orders. Currency issuance and price discovery remain separate
policies.

```cpp
using namespace StonePotatoes::Civilization::Economic;

std::vector<MarketAccount> accounts(2);
accounts[0].inventory = {{"grain", 30.0f}, {"ore", 5.0f}};
accounts[1].funds = 100.0f;
auto result = SettleCommodityTrades(accounts, {
    {0, 1, "grain", 10.0f, 2.0f},
    {0, 1, "ore", 5.0f, 8.0f},
});
```

The API throws `std::invalid_argument` for invalid accounts or orders and
`std::overflow_error` if the settled output cannot be represented by its
float-valued balances. It does not model credit, delivery delays, taxes,
currency conversion, currency issuance, or price discovery.
