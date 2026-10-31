# Plurality election and succession primitives

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

## Validated individual ballots

`TallyPluralityBallots` supplies the small voter-to-tally path for callers that
have already determined eligible voters and declared candidates. It rejects
duplicate IDs, ineligible or repeated voters, and ballots for undeclared
candidates, then delegates winner and tie behavior to the deterministic tally.
It does not define voter eligibility, districts, secret-ballot storage, or
tie-breaking policy.

```cpp
const auto result = TallyPluralityBallots(
    {"candidate-a", "candidate-b"}, {"voter-1", "voter-2"},
    {{"voter-1", "candidate-a"}, {"voter-2", "candidate-b"}});
```

## Leadership succession

`ResolveLeadershipSuccession` applies a succession event after the game has
decided that an office needs a new holder. Election succession consumes the
tally result above; hereditary and appointed succession consume a successor ID
chosen by the caller. An election tie leaves the current holder and term number
unchanged and returns the tied candidates. A resolved succession updates the
holder ID and increments the term number, including reelection of the same
person. The API does not decide when a term expires, who is eligible to inherit,
or how an appointment is made.

```cpp
LeadershipState current{"incumbent", 2};
LeadershipSuccessionRequest request;
request.method = SuccessionMethod::Election;
request.electionResult = result;
const auto succession = ResolveLeadershipSuccession(current, request);
```

## Election term calendar

`BeginElectionTerm` initializes a caller-owned monthly clock with a positive
term length. `AdvanceElectionTerm` reports when the configured term expires and
retains any overdue months until the caller starts a new term. It does not
automatically run an election or succession; after resolving the leadership
change, create a fresh state with `BeginElectionTerm`.

```cpp
auto term = BeginElectionTerm({48});
term = AdvanceElectionTerm(term, elapsedMonths);
if (term.electionDue) {
    // Run the configured election/succession flow, then start the next term.
    term = BeginElectionTerm({48});
}
```
