# 2026-03-23 Tado System Tests and Mode Changes Analysis

Source capture:

- [ebus-2026-03-23-tado-system-tests.log](/Users/jamesadams/git/gshp-ebus/logs/bus-captures/ebus-2026-03-23-tado-system-tests.log)
- [ebus-2026-03-23-hpo-system-test-and-mode-changes.notes.txt](/Users/jamesadams/git/gshp-ebus/logs/bus-captures/ebus-2026-03-23-hpo-system-test-and-mode-changes.notes.txt)
- [2026-03-23-tado-system-tests-time-anchors.csv](/Users/jamesadams/git/gshp-ebus/logs/bus-captures/2026-03-23-tado-system-tests-time-anchors.csv)

Observed real-world events:

- `21:49:00` Tado HPO X `System Test` started from the device menu
- `21:49:10` first `System Test` completed approximately
- `21:49:10` second `System Test` started approximately
- `21:49:20` second `System Test` completed approximately
- `21:51:00` heating mode changed from `Room Guided` to `Heat Pump Guided`
- `21:52:00` heat-pump-guided setpoint increased to `30C`
- `21:53:00` bathroom set to `30C`
- `21:54:00` bathroom set to `Resume Schedule`
- `21:55:00` DHW set to `BOOST`
- `21:59:00` heating mode set back to `Resume Schedule` at `21C`
- `22:00:00` automatic heating schedule change to `18C`
- `22:03:00` heating mode changed from `Heat Pump Guided` back to `Room Guided`

The user observed that the compressor itself did not change state during this sequence because the heat pump was still on a switch-on delay.

## Bus-time anchors

This short run contains the confirmed Vaillant date/time broadcast `AA 10 FE B5 16 08 00 ...` every minute from `21:42:12` to `22:04:12`.

That gives us reliable wall-clock anchors for the two most important windows:

- `21:49:00-21:49:45` for the two back-to-back device-menu `System Test` runs
- `21:59:00-22:03:30` for the `Resume Schedule`, automatic schedule change, and return to `Room Guided`

## Summary

This capture separates two things that had been blurred together before.

The device-menu `System Test` at `21:49` did move some controller-side `B5` families, but it did **not** produce the strong `E0/E1` `07`-type burst that stood out in the previous labelled run.

The later `21:59:55-22:00:04` transition **did** produce that burst. In this run, the best candidate for the `E0/E1` handshake is therefore the `Resume Schedule` and schedule-mode transition, not the short device-menu `System Test` itself.

The strongest state-family candidate remains `AA 1F 08 B5 11 01 00 41`, especially when paired with its slower mirror `AA 10 08 B5 11 01 00 88`.

## Most useful findings

### 1. The device-menu `System Test` did not trigger the `E0/E1` `07` burst

In the `21:48:45-21:49:45` window there are no `AA 10 E0 07 ...`, `AA 03 E0 07 ...`, `AA 10 E1 07 ...`, or `AA 03 E1 07 ...` frames.

What did move in that window was the `11 01 00` pair:

```text
21:49:06.451  AA 1F 08 B5 11 01 00 41 00 09 15 02 17 00 00 08 00 01 00 1B 00
21:49:13.903  AA 10 08 B5 11 01 00 88 00 09 15 02 17 00 00 08 00 01 00 1B 00
```

and the usual paired `1F->08 B5 1A 04 05` loop remained present:

```text
21:49:14.570  AA 1F 08 B5 1A 04 05 FF 32 24 BD 00 0A FF 08 35 00 00 00 00 00 00 00 5B 00
21:49:15.687  AA 1F 08 B5 1A 04 05 FF 32 23 BA 00 0A FF 08 35 00 00 00 00 00 00 00 5B 00
```

Interpretation, marked as inference:

- the device-menu `System Test` is not the same bus action as the later app-driven schedule or mode changes
- it still appears to nudge controller-state families, but more quietly

### 2. `1F->08 B5 11 01 00` is the best current controller-state candidate

Across the user-labelled actions, this family and its `10->08` mirror step through a very clear sequence.

Representative first frames after each action:

| Event | `1F->08 B5 11 01 00` | `10->08 B5 11 01 00` |
| --- | --- | --- |
| `21:49` device-menu `System Test` | `... 15 02 ... 1B 00` | `... 15 02 ... 1B 00` |
| `21:51` `Heat Pump Guided` | `... 1C 02 ... AF 00` | `... 1C 02 ... AF 00` |
| `21:52` setpoint `30C` | `... 19 02 ... EB 00` | `... 19 02 ... EB 00` |
| `21:53` bathroom `30C` | `... 12 02 ... FE 00` | `... 14 02 ... 86 00` |
| `21:54` bathroom resume | `... 10 02 ... 5F 00` | `... 0E 02 ... 5C 00` |
| `21:55` DHW `BOOST` | `... 0B 02 ... 18 00` | `... 09 02 ... 03 00` |
| `21:59` resume schedule `21C` | `... F2 01 ... 73 00` | `... F2 01 ... 73 00` |
| `22:00` auto schedule `18C` | `... F0 01 ... 8C 00` | `... F0 01 ... 8C 00` |
| `22:03` back to `Room Guided` | later `... EC 01 ... 2E 00` | `... EC 01 ... 2E 00` |

Interpretation, marked as inference:

- bytes around positions `10-12` and `19` are very likely encoding a controller target, demand level, or effective operating request
- the `1F` and `10->08` versions are close enough that they now look like the same underlying value block seen on different cadences

This is one of the clearest pieces of evidence we have so far.

### 3. The strong `E0/E1` `07` burst belongs to the later schedule or mode transition

The most obvious `07`-type cluster in this run is:

```text
21:59:55.215  AA 10 E0 07 04 00 6A
21:59:55.744  AA 03 E0 07 04 00 89
21:59:57.251  AA 10 E0 07 04 00 6A
21:59:57.779  AA 03 E0 07 04 00 89
21:59:58.677  AA 10 E1 07 04 00 34
21:59:59.207  AA 03 E1 07 04 00 D7
21:59:59.972  AA 10 E1 07 04 00 34
22:00:00.501  AA 03 E1 07 04 00 D7
22:00:01.265  AA 10 E1 07 04 00 34
22:00:01.794  AA 03 E1 07 04 00 D7
```

This is almost exactly the kind of burst seen in the earlier labelled `2026-03-22` run, but here it lines up with the `Resume Schedule` and `22:00` automatic schedule change rather than the `21:49` device-menu test.

Interpretation, marked as inference:

- this burst is more likely to be an active Tado control or schedule handshake than a generic `System Test` signature
- route `03` remains especially interesting because it appears mainly inside this short exchange

### 4. The `B5 10` families are now good candidates for direct mode or enable markers

The ordinary background forms in this run look like:

```text
AA 10 08 B5 10 09 00 00 42 FF FF FF 06 00 00 DD 00 01 01 9A 00
AA 10 76 B5 10 09 00 00 00 FF FF FF 05 00 00 DD 00 01 01 9A 00
```

During the later mode and schedule churn, new variants appeared:

```text
21:59:02.266  AA 10 08 B5 10 09 00 03 FF FF 78 FF 07 00 01 06 00 01 01 9A 00
22:00:03.362  AA 10 08 B5 10 09 00 03 FF FF 78 FF 07 00 01 06 00 01 01 9A 00
22:03:12.893  AA 10 08 B5 10 09 00 01 00 FF FF FF 06 00 01 93 00 01 01 9A 00
22:03:13.165  AA 10 76 B5 10 09 00 03 FF FF 78 FF 01 00 01 D1 00 01 01 9A 00
22:03:22.243  AA 10 08 B5 10 09 00 01 00 FF FF FF 06 00 01 93 00 01 01 9A 00
22:03:22.584  AA 10 76 B5 10 09 00 03 FF FF 78 FF 01 00 01 D1 00 01 01 9A 00
```

Interpretation, marked as inference:

- these are now some of the best candidates for explicit `mode`, `request`, or `enable` markers
- the `00 03` and `00 01` variants look much more like discrete state transitions than the slower temperature-like families do

### 5. `1F->08 B5 1A 04 05` changed character after the return to `Room Guided`

Before the `22:03` return to `Room Guided`, this family kept its familiar alternating pair:

```text
22:02:59.867  AA 1F 08 B5 1A 04 05 FF 32 24 BD ...
22:03:02.718  AA 1F 08 B5 1A 04 05 FF 32 24 BD ...
```

Immediately after the `22:03` change, it became much more one-sided:

```text
22:03:05.379  AA 1F 08 B5 1A 04 05 FF 32 24 BD ...
22:03:08.041  AA 1F 08 B5 1A 04 05 FF 32 24 BD ...
22:03:10.705  AA 1F 08 B5 1A 04 05 FF 32 24 BD ...
22:03:13.477  AA 1F 08 B5 1A 04 05 FF 32 24 BD ...
22:03:16.139  AA 1F 08 B5 1A 04 05 FF 32 24 BD ...
22:03:21.195  AA 1F 08 B5 1A 04 05 FF 32 23 BA ...
```

Interpretation, marked as inference:

- this family may be representing a controller-side sub-state that can latch for a while
- it still looks like a valuable status family, but probably not a simple on or off flag by itself

## Best current interpretation

This run moves us closer to human-readable state in three practical ways.

First, it strengthens `AA 1F 08 B5 11 01 00 41` as the best candidate for a controller-demand or effective-target family.

Second, it narrows the `E0/E1` `07` burst to schedule or mode transitions rather than treating it as a generic `System Test` marker.

Third, it makes the `B5 10` variants much more interesting, because they now look like the most discrete mode-like transitions in the whole capture.

## Recommended next decode targets

Use this capture to focus on:

1. `AA 1F 08 B5 11 01 00 41`
   - likely controller target or demand state
2. `AA 10 08 B5 11 01 00 88`
   - likely mirror or plant-facing view of the same state block
3. `AA 10 E0 07 04 00 6A`
   - `AA 03 E0 07 04 00 89`
   - `AA 10 E1 07 04 00 34`
   - `AA 03 E1 07 04 00 D7`
   - likely short control or schedule handshake
4. `AA 10 08 B5 10 09 ...`
   - especially the new `00 03` and `00 01` variants
5. `AA 10 76 B5 10 09 ...`
   - especially the new `00 03` variant

If the goal is the shortest path to a readable state like `controller request active`, this capture is more valuable than another blind overnight run.
