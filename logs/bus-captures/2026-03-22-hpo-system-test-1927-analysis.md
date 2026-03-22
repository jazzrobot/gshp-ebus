# 2026-03-22 HPO System Test Analysis

Source capture:

- [ebus-2026-03-22-hpo-system-test-1927.log](/Users/jamesadams/git/gshp-ebus/logs/bus-captures/ebus-2026-03-22-hpo-system-test-1927.log)

Observed real-world events:

- `2026-03-22 19:27` compressor started
- `2026-03-22 19:28` Tado HPO X `System Test` started

## Bus-time anchors

The capture contains on-bus date/time broadcasts:

- `74108248097 us` -> `2026-03-22 19:27:25`
- `74168259067 us` -> `2026-03-22 19:28:25`

That gives a reliable wall-clock anchor for the frames below.

## Summary

This was worth capturing.

The strongest signal from the Tado system test is not a dramatic change in the normal `B5` telemetry loops, but a short clustered burst of `07`-type exchanges involving `E0` and `E1` routes. Those packets appear in the overnight baseline too, but here they occur in a tight group roughly `18-25 s` after the `19:28:25` bus-time anchor, which makes them a good candidate for an active test or handshake phase rather than ordinary plant telemetry.

Because the compressor had already started at `19:27`, the slower `B5` families are harder to attribute cleanly to the Tado system test alone. They still moved, but they were already in an active heating cycle.

## Most useful findings

### 1. Short `07`-type handshake traffic clustered after the test start

Roughly `19:28:43` to `19:28:54`, the capture shows:

- `AA 10 E0 07 04 00 6A`
- `AA 03 E0 07 04 00 89`
- `AA 10 E1 07 04 00 34`
- `AA 03 E1 07 04 00 D7`

Examples:

```text
19:28:43.299  AA 1F 08 B5 11 01 00 41 ...
19:28:43.342  AA 10 E0 07 04 00 6A
19:28:43.871  AA 03 E0 07 04 00 89
19:28:44.539  AA 1F 08 B5 11 01 01 40 ...
19:28:44.620  AA 10 E0 07 04 00 6A
19:28:45.149  AA 03 E0 07 04 00 89
19:28:45.903  AA 10 E0 07 04 00 6A
19:28:46.432  AA 03 E0 07 04 00 89
19:28:47.295  AA 10 08 B5 11 01 01 89 ...
19:28:47.296  AA 03 E1 07 04 00 D7
19:28:48.047  AA 10 E1 07 04 00 34
19:28:48.619  AA 03 E1 07 04 00 D7
19:28:49.941  AA 10 E1 07 04 00 34
19:28:50.469  AA 03 E1 07 04 00 D7
```

Interpretation:

- These look like a deliberate protocol exchange.
- They are much more burst-like than the steady `~10 s`, `~24 s`, and `~60 s` families.
- `03` is especially interesting because it is not one of the main long-lived routes we have been focusing on.

Marked as inference:

- this is currently the best candidate for a Tado-specific active test handshake
- but it is not yet safe to say that route `03` is definitely Tado without another labelled run

### 2. `1F->08 B5 11 01 00` moved strongly during the test window

Representative frames:

```text
19:28:19.045  AA 1F 08 B5 11 01 00 41 00 09 21 02 15 64 00 09 00 01 C8 BF 00
19:28:43.299  AA 1F 08 B5 11 01 00 41 00 09 2A 02 16 64 00 09 00 01 C8 36 00
19:29:09.572  AA 1F 08 B5 11 01 00 41 00 09 2F 02 16 64 00 09 00 01 C8 72 00
```

Important bytes that changed:

- byte 10
- byte 12
- byte 19

Interpretation:

- This remains the strongest current controller-state candidate.
- It changed during the system-test window, not just during the earlier compressor start.

### 3. `1F->08 B5 1A 04 05` showed a repeating two-state pattern

Representative frames:

```text
19:28:27.229  AA 1F 08 B5 1A 04 05 FF 32 24 BD 00 0A FF 08 35 1D 00 00 00 00 00 00 45 00
19:28:28.369  AA 1F 08 B5 1A 04 05 FF 32 23 BA 00 0A FF 08 35 6C 00 00 00 00 00 00 02 00
19:28:53.532  AA 1F 08 B5 1A 04 05 FF 32 24 BD 00 0A FF 08 35 1D 00 00 00 00 00 00 45 00
19:28:54.783  AA 1F 08 B5 1A 04 05 FF 32 23 BA 00 0A FF 08 35 6C 00 00 00 00 00 00 02 00
```

Important bytes that changed:

- byte 9
- byte 10
- byte 16
- byte 23

Interpretation:

- This family again looks very sensitive to controller or test state.
- The repeating paired pattern makes it a good candidate for a diagnostic phase marker.

### 4. `1F->08 B5 11 01 01` still mirrors the Vaillant-side `10->08 B5 11 01 01`

Representative frames:

```text
19:28:20.168  AA 1F 08 B5 11 01 01 40 00 09 44 39 00 0C FF 42 01 00 39 FD 00
19:28:44.539  AA 1F 08 B5 11 01 01 40 00 09 45 39 D0 0B FF 42 01 00 3A D8 00

19:28:24.394  AA 10 08 B5 11 01 01 89 00 09 44 39 D0 0B FF 42 01 00 39 46 00
19:28:47.295  AA 10 08 B5 11 01 01 89 00 09 45 39 D0 0B FF 42 01 00 3A D8 00
```

Interpretation:

- This still looks like a mirror or near-mirror of the Vaillant operational block.
- It moved, but the movement is not uniquely attributable to the Tado system test because the compressor was already running.

### 5. `10->08 B5 10 09 00 00` stayed mostly stable until later in the window

Representative frames:

```text
19:28:26.777  AA 10 08 B5 10 09 00 00 41 FF FF FF 06 00 00 41 00 01 01 9A 00
19:29:10.509  AA 10 08 B5 10 09 00 00 42 FF FF FF 06 00 00 DD 00 01 01 9A 00
```

Interpretation:

- This still does not look like the immediate edge indicator for the test start.
- It may reflect a later system or operating-state consequence instead.

## Noise and framing artefacts

There are a few short imperfect frames in this capture:

- `AA 00`
- `00 00 00`
- one `1F->15` frame truncated to length `24`

These look like framing artefacts rather than meaningful protocol traffic.

## Best current interpretation

Marked as inference:

- the Tado `System Test` did produce a useful on-bus signature
- the cleanest candidate is the clustered `E0/E1` `07`-type exchange
- `1F->08 B5 11 01 00` and `1F->08 B5 1A 04 05` remain the best `B5` families to watch for Tado-originated state
- the temperature-like `11 01 01` families are still informative, but this run cannot fully separate test effects from the already-active compressor cycle

## Recommended next run

For a cleaner Tado-labelled capture, run the same `System Test` again when:

- the compressor is not already running
- the system has been quiet for at least a couple of minutes

That should make it much easier to answer:

- whether the `E0/E1` `07` burst is always tied to the test
- whether `1F->08 B5 11 01 00` flips immediately even without thermal response
- whether `1F->08 B5 1A 04 05` is a pure controller/test-state family
