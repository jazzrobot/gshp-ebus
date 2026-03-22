# 2026-03-22 Compressor Cycle Analysis

This note analyses the short labelled compressor event captured in:

- [ebus-2026-03-22-compressor-cycle-1907-1910-tail.log](ebus-2026-03-22-compressor-cycle-1907-1910-tail.log)
- [ebus-2026-03-22-19h06-19h11.log](ebus-2026-03-22-19h06-19h11.log)
- [ebus-2026-03-22-compressor-cycle-1907-1910.notes.txt](ebus-2026-03-22-compressor-cycle-1907-1910.notes.txt)

## Event markers

User-observed plant events:

- `2026-03-22 19:07` compressor start
- `2026-03-22 19:10` compressor stop

These are minute-resolution human observations, not exact bus timestamps.

## Bus-time anchors

The short capture contains the confirmed Vaillant date/time broadcast `AA 10 FE B5 16 08 00 ...`, which gives minute-by-minute on-bus wall-clock anchors:

- `19:06:25` at `72848362563 us`
- `19:07:25` at `72908422979 us`
- `19:08:25` at `72968416749 us`
- `19:09:25` at `73028367387 us`
- `19:10:25` at `73088860176 us`

Using linear interpolation between those anchors gives approximate event boundaries:

- compressor start at about `72883397806 us`
- compressor stop at about `73063654847 us`

For this note, three comparison windows were used:

- pre: about `19:06:00` to `19:07:00`
- run: about `19:07:00` to `19:10:00`
- post: about `19:10:00` to `19:11:00`

## Strong findings

### `1F->08 B5 11 01 01` mirrors `10->08 B5 11 01 01`

This is now one of the clearest results in the project so far.

Before the compressor start:

```text
AA 10 08 B5 11 01 01 89 00 09 36 35 70 0C FF 42 00 00 37 69 00
AA 1F 08 B5 11 01 01 40 00 09 36 35 70 0C FF 42 00 00 37 69 00
```

During the run:

```text
AA 10 08 B5 11 01 01 89 00 09 39 35 70 0C FF 42 01 00 37 B3 00
AA 10 08 B5 11 01 01 89 00 09 42 36 70 0C FF 42 01 00 37 62 00

AA 1F 08 B5 11 01 01 40 00 09 39 35 70 0C FF 42 01 00 37 B3 00
AA 1F 08 B5 11 01 01 40 00 09 43 37 70 0C FF 42 01 00 38 26 00
```

After the stop:

```text
AA 10 08 B5 11 01 01 89 00 09 45 39 70 0C FF 42 00 00 3A CF 00
AA 1F 08 B5 11 01 01 40 00 09 45 38 70 0C FF 42 01 00 3A 0F 00
```

Interpretation:

- the `1F->08` family is not just loosely similar to the `10->08` family
- it is carrying essentially the same downstream payload block on a different polling schedule
- both families track the same rising plant values through the compressor cycle

That strongly supports the idea that `1F` is another controller or supervisory participant asking for the same Vaillant-side data.

### `10->76 B5 11 01 01` moves with the same event

Representative frames:

```text
pre:  AA 10 76 B5 11 01 01 16 00 09 FF 35 70 0C FF 42 00 01 FF 60 00
run:  AA 10 76 B5 11 01 01 16 00 09 FF 37 70 0C FF 42 00 01 FF 57 00
post: AA 10 76 B5 11 01 01 16 00 09 FF 39 70 0C FF 42 00 01 FF D2 00
```

This looks like the same physical cycle reflected through a different target address.

### `1F->08 B5 11 01 00` changes much more sharply than the plain temperature-like families

Representative frames:

```text
pre:  AA 1F 08 B5 11 01 00 41 00 09 B4 01 16 00 00 08 00 01 00 03 00
run:  AA 1F 08 B5 11 01 00 41 00 09 C5 01 15 64 00 09 00 01 C8 CD 00
post: AA 1F 08 B5 11 01 00 41 00 09 28 02 16 64 00 09 00 01 C8 97 00
```

This family did not merely drift in the same gentle way as the temperature-like blocks. It switched shape across the event boundary and is therefore a strong candidate for controller-side state rather than simple sensed temperature.

### `1F->08 B5 1A 04 05` also reacts clearly to the run state

Before the start:

```text
AA 1F 08 B5 1A 04 05 FF 32 23 BA 00 0A FF 08 35 00 00 00 00 00 00 00 5B 00
AA 1F 08 B5 1A 04 05 FF 32 24 BD 00 0A FF 08 35 01 00 00 00 00 00 00 A6 00
```

During the run:

```text
AA 1F 08 B5 1A 04 05 FF 32 24 BD 00 0A FF 08 35 17 00 00 00 00 00 00 3B 00
AA 1F 08 B5 1A 04 05 FF 32 23 BA 00 0A FF 08 35 82 00 00 00 00 00 00 51 00
```

After the stop:

```text
AA 1F 08 B5 1A 04 05 FF 32 24 BD 00 0A FF 08 35 1C 00 00 00 00 00 00 B8 00
AA 1F 08 B5 1A 04 05 FF 32 23 BA 00 0A FF 08 35 00 00 00 00 00 00 00 5B 00
```

The bytes after the fixed `08 35` region are good candidates for a run-state, stage, or counter field.

### `10->08 B5 10 09 00 00` is not an immediate compressor-edge indicator

This family stayed stable through almost the entire run:

```text
AA 10 08 B5 10 09 00 00 3E FF FF FF 06 00 00 BA 00 01 01 9A 00
```

Only around `+45 s` after the stop boundary did it move to:

```text
AA 10 08 B5 10 09 00 00 3F FF FF FF 06 00 00 47 00 01 01 9A 00
```

That makes it look more like a target or slower supervisory value than a direct compressor-state flag.

### `1F->08 B5 03 02 00 01` stayed static

Across the whole event window, this family remained:

```text
AA 1F 08 B5 03 02 00 01 6E 00 0A 60 04 FF FF FF FF FF FF FF FF 5B 00
```

That lowers its priority for compressor-state decoding.

## Important caution about `1F->15 B5 24 05 03`

At first glance, `AA 1F 15 B5 24 05 03 01` and `AA 1F 15 B5 24 05 03 03` looked like possible compressor-on markers because they were present in the run window and absent from the one-minute post window.

That interpretation is too strong.

The same families also appear earlier in this short tail, before the user-noted `19:07` start time, for example around:

- `72556491500 us` to `72566308398 us`

So these are real recurring families, but not clean compressor-only indicators from this capture alone.

## Current best decode priorities after this event

1. `AA 1F 08 B5 11 01 00`
   - strongest controller-state candidate from this event

2. `AA 1F 08 B5 1A 04 05`
   - clearly responsive to run-state changes

3. `AA 1F 15 B5 24 06 02`
   - still the dominant live family, but it needs a more structured field map

4. `AA 10 08 B5 11 01 01` and `AA 1F 08 B5 11 01 01`
   - strong pair for calibrating temperature-like fields

## Practical next step

The best next labelled event is not another ordinary compressor cycle. It is a Tado-side state change with a short capture around it:

- raise setpoint
- lower setpoint
- toggle DHW if exposed
- change heating mode if exposed

Then re-check:

- `1F->08 B5 11 01 00`
- `1F->08 B5 1A 04 05`
- `1F->15 B5 24 06 02`

This compressor event strongly suggests that the `1F->08` families are where controller-intent and controller-state decoding will pay off first.
