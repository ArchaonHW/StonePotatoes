# Currency Supply Model

`StonePotatoes::Civilization::Economic::BuildCurrencySupplyModel` provides an
installable monthly aggregate supply model. Its stock records total units of
one currency; configured monthly issuance adds units up to `maximumSupply`,
then configured monthly burn removes units from the resulting supply. Burn is
capped at available supply, so the balance cannot become negative.

Issuance and burn amounts can be adjusted between steps using
`CurrencyIssuanceParameterName()` and `CurrencyBurnParameterName()`. Invalid
initial configuration is rejected. Non-finite or negative runtime parameter
values are treated as zero. This model does not allocate balances to accounts,
set monetary policy from inflation, or model exchange rates; use it as an
aggregate supply primitive alongside the market settlement model.

```cpp
using namespace StonePotatoes::Civilization::Economic;

CurrencySupplyConfig config;
config.initialSupply = 1000.0f;
config.monthlyIssuance = 20.0f;
config.monthlyBurn = 5.0f;
config.maximumSupply = 5000.0f;
auto currency = BuildCurrencySupplyModel(config);
currency.Step();
```
