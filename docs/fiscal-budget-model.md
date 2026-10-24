# Civilization Fiscal Budget Model

The installed `StonePotatoes::Civilization` target provides
`civilization/economic/FiscalBudget.h` for a deterministic monthly public
treasury budget.

The model takes an externally supplied monthly taxable output and a tax rate in
`[0, 1]`. Tax revenue is credited to the treasury in the same step. Public
spending is then charged up to the available treasury; any request above that
amount is capped, so the model does not create an implicit deficit or debt.
Tax base, rate, and spending can be changed between steps with `SetParam`.
Each step represents one month; keep the model's default `dt` of `1` so the
monthly flow and spending limits retain their stated meaning.

```cpp
using namespace StonePotatoes::Civilization::Economic;

FiscalBudgetConfig config;
config.initialTreasury = 20.0f;
config.monthlyTaxBase = 100.0f;
config.taxRate = 0.1f;
config.monthlyPublicSpending = 8.0f;

auto budget = BuildFiscalBudgetModel(config);
budget.Step(); // treasury becomes 22: 20 + 10 tax - 8 spending
const float treasury = budget.Stock(FiscalTreasuryStockName());
```

The tax base is an aggregate input: this slice does not model household or
business accounts, tax incidence, transfers, borrowing, interest, or fiscal
policy behavior. It is a building block for strategy simulation, not a full
political-economy system.
