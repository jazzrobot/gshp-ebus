# 2026-03-22 to 2026-03-23 Overnight Comparison

This note compares the second large overnight capture with the previous overnight run.

Source files:

- [2026-03-21 to 2026-03-22 raw log](/Users/jamesadams/git/gshp-ebus/logs/bus-captures/ebus-2026-03-21-to-2026-03-22-overnight.log)
- [2026-03-22 to 2026-03-23 raw log archive](/Users/jamesadams/git/gshp-ebus/logs/bus-captures/ebus-2026-03-22-to-2026-03-23-overnight.log.gz)
- [2026-03-22 to 2026-03-23 8-byte summary](/Users/jamesadams/git/gshp-ebus/logs/bus-captures/2026-03-22-to-2026-03-23-overnight-summary.md)
- [2026-03-22 to 2026-03-23 7-byte logical summary](/Users/jamesadams/git/gshp-ebus/logs/bus-captures/2026-03-22-to-2026-03-23-overnight-logical-summary-7b.md)
- [2026-03-22 to 2026-03-23 time anchors](/Users/jamesadams/git/gshp-ebus/logs/bus-captures/2026-03-22-to-2026-03-23-overnight-time-anchors.csv)

## File scope

The new raw capture contains:

- `635,646` raw lines
- `111 MB`

Important note:

- the `up_ms` value in the final firmware stats is device uptime, not guaranteed file-span duration
- for file-span timing, the time-anchor CSV is the more reliable reference

Based on the first and last minute anchors in the new file, the capture spans roughly:

- start: `2026-03-22 19:00:25`
- end: `2026-03-23 21:36:12`
- file span: about `26 h 35 m 47 s`

## High-level comparison

### Previous overnight run

- raw lines: `475,135`
- parsed frame lines: `117,437`
- 8-byte keys: `127`
- 7-byte keys: `66`

### New overnight run

- raw lines: `635,646`
- parsed frame lines: `157,072`
- 8-byte keys: `141`
- 7-byte keys: `81`

## Main conclusion

The second overnight run does **not** reveal a radically different traffic mix.

Instead, it reinforces the same overall picture:

- the dominant `1F->15 B5 24 06 02` family remains dominant
- the `~10 s` Vaillant-style `10->08` and `10->76` groups remain stable
- the `~24 s` `1F->08` groups remain stable
- the `~60 s` configuration and time families remain stable

What changed is mostly:

- more total examples
- more rare variants
- more observed length variants
- better confidence that the main groups are real long-lived behaviours rather than short-term quirks

## Dominant family stability

### Previous run

- `AA 1F 15 B5 24 06 02 00`
- `46,047` frames
- `39.21%` of parsed frames
- median gap `1.138 s`

### New run

- `AA 1F 15 B5 24 06 02 00`
- `61,570` frames
- `39.20%` of parsed frames
- median gap `1.138 s`

Interpretation:

- This is essentially unchanged.
- The dominant `1F->15` family is a core background behaviour of the system.

## Stable `~10 s` families

The new run again shows the same long-lived set around `10 s` cadence:

- `10->76 B5 10 09 00`
- `10->76 B5 11 01 01`
- `10->76 B5 12 03 0F`
- `10->08 B5 10 09 00`
- `10->08 B5 11 01 01`

In the new run, each of these sits at roughly `9,500` examples, with median gaps clustered around `9.997–9.998 s`.

Interpretation:

- These look like the most stable Vaillant-side operational families.
- They are excellent anchors for any future decoder work.

## Stable `~24 s` families

The new run again shows:

- `1F->08 B5 03 02 00`
- `1F->08 B5 11 01 00`
- `1F->08 B5 11 01 01`

These still sit near `24.258–24.262 s`.

Interpretation:

- These are still the best controller-side periodic families to watch.
- The second overnight run makes it less likely that they were only tied to a single temporary mode.

## `1F->08 B5 1A 04 05` remains unusual and important

This family again stands out:

- previous run: `5,529` frames, median gap `2.047 s`
- new run: `7,407` frames, median gap `2.610 s`

Interpretation:

- It is clearly not just a simple `24 s` or `10 s` periodic telemetry family.
- It still behaves like a multi-state or paired-state control family.
- This remains one of the strongest non-temperature candidates for controller or test state.

## Rare and odd traffic

The new run still contains:

- short `E0` and `E1` `07`-type traffic
- artefact-like `idle_timeout` fragments
- extra length variants in some major families

Interpretation:

- the protocol surface is larger than the current on-device `24`-family table can represent
- but the rare traffic still does not overturn the main family hierarchy

## What this means for the project

This second overnight run is valuable because it tells us that:

1. our main candidate families are stable across a much longer observation window
2. the top family proportions are consistent
3. we are now well past the point where more blind overnight capture is the main bottleneck

The bottleneck is now interpretation.

## Recommended next step

Use this second overnight run as reinforcement, then focus on labelled perturbations:

- quiet-baseline Tado `System Test`
- clean compressor start from idle
- clean compressor stop
- deliberate Tado setpoint or mode changes

Those labelled runs will add more value than another unlabelled overnight capture of similar quality.
