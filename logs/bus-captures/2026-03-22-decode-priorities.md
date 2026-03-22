# 2026-03-22 Decode Priorities

This note turns the first long unattended capture run into a working decode plan.

It builds on:

- [2026-03-21-protocol-notebook.md](2026-03-21-protocol-notebook.md)
- [2026-03-21-to-2026-03-22-overnight-summary.md](2026-03-21-to-2026-03-22-overnight-summary.md)
- [2026-03-21-to-2026-03-22-overnight-families.csv](2026-03-21-to-2026-03-22-overnight-families.csv)
- [2026-03-21-to-2026-03-22-overnight-logical-summary-7b.md](2026-03-21-to-2026-03-22-overnight-logical-summary-7b.md)
- [2026-03-21-to-2026-03-22-overnight-logical-families-7b.csv](2026-03-21-to-2026-03-22-overnight-logical-families-7b.csv)
- [2026-03-21-to-2026-03-22-overnight-time-anchors.csv](2026-03-21-to-2026-03-22-overnight-time-anchors.csv)

## Key conclusion

Use two views of the same run:

- `8`-byte keys for detailed byte-level comparison inside a family
- `7`-byte keys for higher-level protocol reasoning

The overnight run produced `127` distinct `8`-byte keys but only `66` distinct `7`-byte keys. That confirms the current firmware signature width is useful for byte-level work, but too fine-grained for selecting logical families.

## Route buckets

These percentages are from the `7`-byte logical-family view.

| Route | Frames | Share | Working interpretation |
| --- | ---: | ---: | --- |
| `1F->15` | 49,340 | 42.01% | Dominant unresolved control or telemetry bucket |
| `10->76` | 22,673 | 19.31% | Strong Vaillant anchor traffic |
| `10->08` | 21,676 | 18.46% | Strong Vaillant anchor traffic |
| `1F->08` | 14,181 | 12.08% | Likely second controller polling known Vaillant targets |
| `10->FE` | 3,578 | 3.05% | Vaillant broadcast and utility traffic |

Working model:

- `10->08`, `10->76`, and `10->FE` are the best current Vaillant anchors.
- `1F->08` is likely another controller or supervisory node asking for related data.
- `1F->15` is the most important unresolved bucket and the best candidate for Tado-correlated traffic.

## Strongest current inferences

### `1F` looks like a real controller-side participant

This is the strongest overnight inference.

The `1F->08 B5 11 01 xx` families look like slower mirrors of the already-identified Vaillant-style `10->08 B5 11 01 xx` families.

Examples:

- `AA 10 08 B5 11 01 01 ...` and `AA 1F 08 B5 11 01 01 ...`
- `AA 10 08 B5 11 01 00 ...` and `AA 1F 08 B5 11 01 00 ...`

The payload shapes and changing positions are closely aligned, but the `1F` variants repeat at about `24 s` rather than `10 s`.

In the overnight run, the captured `first_data` and `last_data` examples for these pairs match almost exactly after the leading family-specific byte near the front of the frame. That makes it look as though `1F` is requesting the same underlying value blocks on a slower schedule.

That makes `1F` look much more like another controller or supervisory node than random noise.

### `1F->15` remains the most valuable unknown

`AA 1F 15 B5 24 06 02` is still the most important family in the whole capture:

- `46,047` frames
- `39.21%` of all parsed frames
- median gap `1.138 s`
- lengths `13,15,17,22,23,24,25,26`
- variation at `8,10,12,13,14,15,16,17,18,19,20,21,22,23,24,25,len`

That is exactly the family most likely to reveal live operating state, target changes, or Tado-originated control intent.

### The overnight capture confirms several clean cadence bands

- `~0.74 s`: secondary `1F->15 B5 24 05 03` loop
- `~1.14 s`: dominant `1F->15 B5 24 06 02` loop
- `~2.05 s`: `1F->08 B5 1A 04 05`
- `~10 s`: main `10->08` and `10->76` Vaillant-style operational families
- `~24 s`: `1F->08 B5 03/11` families
- `~60 s`: slower Vaillant housekeeping and broadcast families

These cadence buckets are now stable enough to use as a decoding aid in their own right.

## Confirmed or strong Vaillant anchors

These are the best calibration families because they either match the external Vaillant reference directly or line up closely with it.

| Logical family | Frames | Cadence | Current status | Why it matters |
| --- | ---: | ---: | --- | --- |
| `AA 10 FE B5 16 08 00` | 1,193 | `~60 s` | Confirmed | Date/time broadcast anchor |
| `AA 10 FE B5 16 03 01` | 1,192 | `~60 s` | Confirmed | Outside-temperature broadcast anchor |
| `AA 10 08 B5 11 01 01` | 7,158 | `~10 s` | Strong match | Operational data from target `08` |
| `AA 10 76 B5 11 01 01` | 7,160 | `~10 s` | Strong match | Operational data from target `76` |
| `AA 10 08 B5 10 09 00` | 7,161 | `~10 s` | Strong match | Control or target-value family |
| `AA 10 76 B5 10 09 00` | 7,160 | `~10 s` | Strong match | Control or target-value family |
| `AA 10 08 B5 11 01 00` | 1,193 | `~60 s` | Strong match | Slower operational-data block |
| `AA 10 08 B5 12 02 00` | 1,194 | `~60 s` | Strong match | Known `B5 12` family anchor |

These families should be used to sanity-check any attempt to label temperatures, outside conditions, or hot-water state.

Because the date/time broadcast is confirmed, future runs can also be mapped back to wall-clock time from the bus itself via [2026-03-21-to-2026-03-22-overnight-time-anchors.csv](2026-03-21-to-2026-03-22-overnight-time-anchors.csv).

## Highest-priority unresolved families

These are the best targets for the next decoding session.

| Logical family | Frames | Median gap | Lengths | Vary idx | Why it is high value |
| --- | ---: | ---: | --- | --- | --- |
| `AA 1F 15 B5 24 06 02` | 46,047 | `1.138 s` | `13,15,17,22,23,24,25,26` | `8,10,12,13,14,15,16,17,18,19,20,21,22,23,24,25,len` | Dominant live family and most likely state or control loop |
| `AA 1F 08 B5 1A 04 05` | 5,529 | `2.047 s` | `24,25` | `9,10,16,23,24,len` | Fast recurring family with limited variation, likely status-like |
| `AA 1F 15 B5 24 05 03` | 3,244 | `0.744 s` | `22,23` | `7,9,10,11,17,18,21,22,len` | Secondary fast loop on the same route as the dominant family |
| `AA 1F 08 B5 11 01 01` | 2,806 | `24.263 s` | `20,21,22` | `10,11,12,13,15,16,18,19,20,21,len` | Likely slower mirror of known Vaillant operational data |
| `AA 1F 08 B5 11 01 00` | 2,752 | `24.259 s` | `9,20,21,22` | `9,10,11,12,13,14,15,16,17,18,19,20,21,len` | Same value family as above, different block or sub-mode |
| `AA 1F 08 B5 03 02 00` | 2,746 | `24.258 s` | `10,19,23` | `10,11,12,13,14,15,16,17,18,19,20,21,22,len` | Stable cadence and persistent presence, but still unknown |
| `AA 1F 08 B5 16 08 10` | 232 | `1.162 s` | `30,31` | `10,11,14,19,20,21,22,24,25,26,27,28,29,30,len` | Lower volume, but structurally rich and potentially informative |

## Lower-priority or likely artefact groups

These should not drive the next decoding session.

- `--` and `AA 00`
  - These are almost certainly incomplete provisional frames from `idle_timeout`.
- `10->E0`, `03->E0`, `10->E1`, `03->E1`
  - These repeat, but they are short `07`-type fragments dominated by `idle_timeout` rather than clean long families.
- Rare one-off route fragments such as `10->0A`, `10->18`, `10->26`, `10->35`, `10->38`, `10->50`, `10->52`, `10->75`, `10->A0`, `10->EC`, `10->ED`
  - These are worth keeping, but not worth manual decoding before the main `1F` families.

## Recommended next session

The next best move is a short, deliberately labelled perturbation run rather than another blind long capture.

Suggested order:

1. Record the wall-clock time.
2. Increase the Tado room setpoint by `+1 C`.
3. Wait `2-3` minutes.
4. Decrease it back.
5. If available, toggle DHW once.
6. If available, change heating mode once.
7. Record the exact wall-clock times of each action.

Watch first:

- `AA 1F 15 B5 24 06 02`
- `AA 1F 15 B5 24 05 03`
- `AA 1F 08 B5 1A 04 05`
- `AA 1F 08 B5 11 01 00`
- `AA 1F 08 B5 11 01 01`
- `AA 1F 08 B5 03 02 00`

Use the confirmed `10->...` Vaillant anchors as controls:

- if a value changes there and matches a known physical condition, it helps calibrate the run
- if only the `1F->...` families react immediately to Tado actions, that is our best evidence yet that `1F` is Tado-related

## Practical rule going forward

For analysis notes and prioritisation:

- prefer the `7`-byte logical-family CSV

For byte-level comparison and field guessing:

- drop back to the `8`-byte family CSV and the raw log
