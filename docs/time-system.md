# Time system

`StonePotatoes::Time::TimeManager` supports normal wall-clock updates through
`Update()` and reproducible simulation updates through `Advance(realDeltaSeconds)`.
The explicit-delta call rejects negative or non-finite input without changing
state. `GetTotalTime()` accumulates unscaled real seconds; `GetDeltaTime()` is
scaled simulation seconds.

At scale `1`, one real second advances one cumulative game minute. The calendar
uses fixed simulation units: 60 minutes per hour, 24 hours per day, 30 days per
month, and 12 months per year. Values returned by `GetGameMinute()` through
`GetGameYear()` are cumulative totals, not clock fields. `SetTimeScale()` accepts
finite non-negative values; zero pauses simulation time. Invalid time scales
and fixed deltas are ignored. `SetFixedDeltaTime()` stores a validated step
size; the caller still owns fixed-step accumulation and scheduling.

The explicit delta API is suitable for deterministic tests and replay inputs
when callers supply the same deltas and time scale. It does not by itself
provide cross-platform lockstep guarantees.
