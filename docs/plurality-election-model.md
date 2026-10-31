# Plurality election tally

`StonePotatoes::Civilization::Politics::TallyPluralityElection` counts
pre-aggregated votes and returns the candidate with the most votes. Candidate
IDs must be nonempty and unique, at least one vote must be present, and total
votes are checked for `uint64_t` overflow.

```cpp
using namespace StonePotatoes::Civilization::Politics;

const auto result = TallyPluralityElection({
    {"candidate-a", 52},
    {"candidate-b", 31},
    {"candidate-c", 17},
});
if (result.winnerCandidateId) {
    // Apply game-specific office succession here.
}
```

An equal top vote count produces no winner. `tiedCandidates` contains every
candidate at that count in lexicographic ID order, so callers can apply an
explicit game rule such as a runoff or appointment. This primitive does not
model voter eligibility, ballots, districts, campaigning, or tie-breaking.
