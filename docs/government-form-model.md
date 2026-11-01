# Government form rules

`StonePotatoes::Civilization::Politics::BuildGovernmentRules` maps a generic
succession form to the existing leadership succession API and optional term
length. It intentionally does not model voter eligibility, the identity of an
heir, appointment procedures, or political ideology.

| Form | Succession method | Term rule |
|---|---|---|
| `Electoral` | `Election` | Positive fixed term required |
| `Hereditary` | `Hereditary` | No fixed term |
| `Appointed` | `Appointment` | Positive fixed term or indefinite |

```cpp
using namespace StonePotatoes::Civilization::Politics;

const auto rules = BuildGovernmentRules(
    {GovernmentForm::Electoral, 48});
// Pass rules.successionMethod into LeadershipSuccessionRequest.
// BeginElectionTerm({*rules.leadershipTermMonths}) when an electoral term starts.
```

The mapping is a small caller-configurable game rule primitive, not a complete
taxonomy of real-world governments. Invalid term combinations throw
`std::invalid_argument`.
