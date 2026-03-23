# 2026-03-23 Tado System Tests, Demand, Compressor and Reset Analysis

Source capture:

- [ebus-2026-03-23-tado-system-tests-2.log](/Users/jamesadams/git/gshp-ebus/logs/bus-captures/ebus-2026-03-23-tado-system-tests-2.log)
- [ebus-2026-03-23-tado-system-tests-2.notes.txt](/Users/jamesadams/git/gshp-ebus/logs/bus-captures/ebus-2026-03-23-tado-system-tests-2.notes.txt)
- [2026-03-23-tado-system-tests-2-time-anchors.csv](/Users/jamesadams/git/gshp-ebus/logs/bus-captures/2026-03-23-tado-system-tests-2-time-anchors.csv)

Observed real-world events:

- `22:14` heating optimisation changed from `Standard` to `Comfort`
- `22:18` large clunk heard; possible restart, but the panels did not reboot
- `22:27` heating optimisation changed from `Comfort` back to `Standard`
- `22:30` app action `Turn heating off All rooms`
- `22:34` app action `resume schedule all rooms`
- `22:37:30` app action `Boost Heating` for all rooms
- `22:39` two Nest-controlled zones engaged as well
- `22:41` heat-pump compressor started
- `22:47` Nest demand stopped
- `22:48` Tado demand stopped
- `22:53` heat-pump compressor stopped
- `22:55` immersion-heater breaker restored and `F.1120` disappeared immediately
- `22:57` side controller reset manually
- `23:00` capture snapshotted and rotated

## Bus-time anchors

This run contains the confirmed Vaillant date/time broadcast `AA 10 FE B5 16 08 00 ...` every minute from `22:05:12` through `23:00:12`.

That gives a reliable wall-clock reference for all of the windows above.

## Summary

This run is much more useful than it first looks.

The app-only actions between `22:14` and `22:37:30` barely moved the main tracked families. The stronger changes appeared only when there was an actual hydraulic or plant-side consequence:

- the Nest-assisted buffer draw at `22:39`
- the compressor start at `22:41`
- the manual side-controller reset at `22:57`

The strongest new conclusion is that the short `E0/E1` `07` burst is **not** just a Tado schedule-change marker. The side-controller reset produced the same burst at `22:57:23-22:57:29`, which makes it look much more like a generic controller-side handshake or renegotiation.

The other important result is that a real compressor start is clearly visible in the bus traffic, but still not yet as a single clean boolean. The best current candidates remain:

- `AA 1F 08 B5 11 01 00 41`
- `AA 10 08 B5 11 01 00 88`
- `AA 1F 08 B5 1A 04 05 FF`

## Most useful findings

### 1. `Standard -> Comfort -> Standard` was almost a no-op on the main tracked families

The `22:14` optimisation change did not produce:

- any `E0/E1` `07` burst
- any new `B5 10` mode variant
- any clear jump in the `11 01 00` or `11 01 01` families

Representative frames before and after `22:14` are effectively unchanged:

```text
22:13:49.629  AA 1F 08 B5 11 01 00 41 00 09 E9 01 13 00 00 18 00 01 00 6A 00
22:14:14.405  AA 1F 08 B5 11 01 00 41 00 09 E9 01 13 00 00 18 00 01 00 6A 00

22:13:50.754  AA 1F 08 B5 11 01 01 40 00 09 3D 3C 50 0A FF 3D 00 00 3C BE 00
22:14:15.489  AA 1F 08 B5 11 01 01 40 00 09 3D 3C 50 0A FF 3D 00 00 3C BE 00
```

The return to `Standard` at `22:27` was equally quiet.

Interpretation, marked as inference:

- the heating-optimisation setting is either very slow to propagate, only affects higher-level curve logic that did not become active during this interval, or is not exposed directly in the spontaneous telegrams we are watching

### 2. The `22:18` clunk does not look like a full reboot

The `22:18` clunk happened without:

- any `E0/E1` `07` burst
- any obviously zeroed controller block
- any dramatic change in the `B5 10` families

There was a modest state shift in the `11 01 00` pair:

```text
22:17:52.757  AA 1F 08 B5 11 01 00 41 00 09 E7 01 13 00 00 08 00 01 00 81 00
22:18:29.990  AA 1F 08 B5 11 01 00 41 00 09 E6 01 16 00 00 08 00 01 00 23 00

22:17:13.922  AA 10 08 B5 11 01 00 88 00 09 E7 01 13 00 00 18 00 01 00 3B 00
22:18:14.048  AA 10 08 B5 11 01 00 88 00 09 E7 01 15 00 00 08 00 01 00 22 00
```

Interpretation, marked as inference:

- this looks more like a small plant or controller-state transition than a full controller restart
- it may have been a relay, valve, or contactor event rather than a communications reset

### 3. `All rooms off`, `resume schedule`, and `Boost Heating` did not cause an immediate control-plane handshake

The app actions at `22:30`, `22:34`, and `22:37:30` did **not** produce the short `E0/E1` `07` burst, and they did not immediately flip the main tracked families.

There are no `07`-type handshake frames in the windows:

- `22:29:40-22:30:40`
- `22:33:40-22:34:40`
- `22:37:10-22:38:10`

Interpretation, marked as inference:

- room-level app demand is still filtered through the Tado HPO logic rather than becoming an immediate, obvious bus-side command
- that fits the developer explanation that the HPO is aggregating room demand and then deciding what to ask from the heat pump

### 4. The first clear response happened when the buffer was actually being pulled down

The first meaningful step after the app actions came when the Nest zones were engaged at `22:39`.

Representative changes:

```text
22:38:40.663  AA 1F 08 B5 11 01 00 41 00 09 E6 01 16 00 00 08 00 01 00 23 00
22:39:17.728  AA 1F 08 B5 11 01 00 41 00 09 E4 01 16 00 00 08 00 01 00 82 00

22:38:41.858  AA 1F 08 B5 11 01 01 40 00 09 3C 3C 50 0A FF 3D 00 00 3B 24 00
22:39:18.841  AA 1F 08 B5 11 01 01 40 00 09 3C 3B 10 0A FF 3D 00 00 3B 53 00

22:38:52.332  AA 10 08 B5 10 09 00 00 5F FF FF FF 06 00 00 C3 00 01 01 9A 00
22:39:02.317  AA 10 08 B5 10 09 00 00 60 FF FF FF 06 00 00 C0 00 01 01 9A 00
```

Interpretation, marked as inference:

- the bus is reacting more clearly to real hydraulic demand than to the abstract app toggles that came just before it
- that again points to Tado acting as a high-level demand shaper rather than a simple relay

### 5. The `22:41` compressor start is the clearest live-state event in this run

This is the strongest labelled compressor window we have so far.

The `11 01 00` families changed decisively:

```text
22:40:58.309  AA 1F 08 B5 11 01 00 41 00 09 B0 01 15 00 00 08 00 01 00 46 00
22:41:22.750  AA 1F 08 B5 11 01 00 41 00 09 96 01 15 00 00 08 00 01 00 9A 00

22:40:14.199  AA 10 08 B5 11 01 00 88 00 09 D9 01 16 00 00 08 00 01 00 19 00
22:41:13.912  AA 10 08 B5 11 01 00 88 00 09 9E 01 15 00 00 08 00 01 00 B3 00
```

The `11 01 01` families also stepped:

```text
22:40:59.444  AA 1F 08 B5 11 01 01 40 00 09 35 32 50 0A FF 3D 00 00 3B 15 00
22:41:23.871  AA 1F 08 B5 11 01 01 40 00 09 32 30 50 0A FF 3D 01 00 3B D1 00

22:40:51.276  AA 10 08 B5 11 01 01 89 00 09 37 32 50 0A FF 3D 00 00 3B B4 00
22:41:11.455  AA 10 08 B5 11 01 01 89 00 09 33 31 50 0A FF 3D 00 00 3B 8C 00
```

The `1A 04 05` family changed from its quiet pair into a much more active one:

```text
22:40:43.318  AA 1F 08 B5 1A 04 05 FF 32 23 BA 00 0A FF 08 35 00 00 00 00 00 00 00 5B 00
22:41:06.577  AA 1F 08 B5 1A 04 05 FF 32 24 BD 00 0A FF 08 35 01 00 00 00 00 00 00 A6 00
22:41:07.726  AA 1F 08 B5 1A 04 05 FF 32 23 BA 00 0A FF 08 35 00 00 00 00 00 00 00 5B 00
22:41:30.875  AA 1F 08 B5 1A 04 05 FF 32 24 BD 00 0A FF 08 35 14 00 00 00 00 00 00 A7 00
22:41:32.139  AA 1F 08 B5 1A 04 05 FF 32 23 BA 00 0A FF 08 35 82 00 00 00 00 00 00 51 00
```

The `B5 10` families did **not** show an equally crisp edge:

```text
22:40:52.160  AA 10 08 B5 10 09 00 00 60 FF FF FF 06 00 00 C0 00 01 01 9A 00
22:41:02.247  AA 10 08 B5 10 09 00 00 60 FF FF FF 06 00 00 C0 00 01 01 9A 00
```

Interpretation, marked as inference:

- the compressor start is very clearly visible
- the best current compressor-state candidates are still the paired `11 01 00` and `1A 04 05` families, not the `B5 10` family
- but they look like mixed state or thermodynamic blocks rather than a single direct `compressor_on` bit

### 6. Stopping demand at `22:47` and `22:48` did not create an immediate off-edge

Stopping the Nest demand and then the Tado demand did not immediately reverse the strong running-state shifts.

Representative frames:

```text
22:46:51.803  AA 1F 08 B5 11 01 00 41 00 09 04 02 15 64 00 09 00 01 C8 5F 00
22:47:16.160  AA 1F 08 B5 11 01 00 41 00 09 09 02 15 64 00 09 00 01 C8 32 00

22:47:40.305  AA 1F 08 B5 11 01 00 41 00 09 0E 02 15 64 00 09 00 01 C8 D7 00
22:48:04.654  AA 1F 08 B5 11 01 00 41 00 09 12 02 15 64 00 09 00 01 C8 75 00
```

Interpretation, marked as inference:

- these families are not pure request bits
- they continue to evolve during the run-down phase, which suggests they mix demand, temperatures, integral, or plant state

### 7. The `22:53` compressor stop is visible, but not as a simple inverse of the start

At the observed stop time, the main tracked families did not snap back. Several continued to rise:

```text
22:52:56.107  AA 1F 08 B5 11 01 00 41 00 09 7D 02 16 64 00 09 00 01 C8 52 00
22:53:20.450  AA 1F 08 B5 11 01 00 41 00 09 87 02 16 64 00 09 00 01 C8 C2 00

22:52:57.229  AA 1F 08 B5 11 01 01 40 00 09 4F 44 50 0A FF 3D 01 00 42 67 00
22:53:21.584  AA 1F 08 B5 11 01 01 40 00 09 51 46 50 0A FF 3D 01 00 42 53 00
```

Interpretation, marked as inference:

- compressor stop is present in the system, but the families we are watching are still dominated by lagging thermal or integral state
- this is why we are close to readable state, but not yet at a one-line `compressor_on` decoder rule

### 8. Restoring the immersion-heater breaker cleared the fault but did not produce a full handshake

The user observed that `F.1120` disappeared immediately at `22:55`.

On the bus, the change was real but modest:

```text
22:54:33.080  AA 1F 08 B5 11 01 00 41 00 09 64 02 16 00 00 08 00 01 00 5E 00
22:55:09.914  AA 1F 08 B5 11 01 00 41 00 09 61 02 17 00 00 08 00 01 00 E7 00

22:55:14.196  AA 10 08 B5 11 01 00 88 00 09 61 02 17 00 00 08 00 01 00 E7 00
22:55:15.508  AA 10 FE B5 16 03 01 50 0A 2C
```

There was **no** `E0/E1` `07` burst here.

Interpretation, marked as inference:

- clearing the immersion-heater phase-failure looks more like a plant-side state correction than a controller communications reset

### 9. The `22:57` side-controller reset produced the cleanest control-plane handshake in the run

This is the strongest non-thermal signature in the whole capture.

Immediately after the reset:

```text
22:57:13.976  AA 10 08 B5 11 01 00 88 00 09 00 00 00 00 00 00 00 00 00 9C 00
22:57:18.770  AA 1F 08 B5 11 01 00 41 00 09 64 02 17 00 00 00 00 01 00 FE 00
22:57:02.264  AA 10 08 B5 10 09 00 00 38 FF FF FF 06 00 00 19 00 01 01 9A 00
```

Then the same `E0/E1` `07` burst seen in earlier labelled runs appeared again:

```text
22:57:23.138  AA 10 E0 07 04 00 6A
22:57:23.667  AA 03 E0 07 04 00 89
22:57:25.172  AA 10 E0 07 04 00 6A
22:57:25.701  AA 03 E0 07 04 00 89
22:57:26.557  AA 10 E1 07 04 00 34
22:57:27.085  AA 03 E1 07 04 00 D7
22:57:27.840  AA 10 E1 07 04 00 34
22:57:28.368  AA 03 E1 07 04 00 D7
22:57:29.133  AA 10 E1 07 04 00 34
22:57:29.661  AA 03 E1 07 04 00 D7
```

This changes the interpretation of that burst.

Interpretation, marked as inference:

- the `E0/E1` `07` burst is better understood as a short controller-side handshake, reset, or renegotiation sequence
- it is **not** unique to a Tado schedule change
- the reset also produced the cleanest temporary zeroing we have yet seen in `AA 10 08 B5 11 01 00 88`

## Best current interpretation

This run sharpens the picture in four useful ways.

First, it confirms that the bus does show a real compressor start very clearly.

Second, it shows that room-level app actions are still too abstract to be trusted as direct plant commands unless they lead to an actual system response.

Third, it weakens the idea that the `E0/E1` `07` burst is a Tado-only schedule marker and strengthens the idea that it is a generic control-plane handshake.

Fourth, it gives us the cleanest controller-reset signature so far.

## What this means for readable states

We are not yet at a single safe rule like:

- `compressor_on = if byte X equals Y`

But we are getting close to a practical multi-family heuristic.

The best current direction for a first human-readable rule is probably:

- `controller_reset_or_handshake_active`
  - driven by the `E0/E1` `07` burst
- `controller_request_or_target_state`
  - driven by `1F->08 B5 11 01 00`
  - cross-checked against `10->08 B5 11 01 00`
- `compressor_running_likely`
  - driven by a combination of `1F->08 B5 11 01 00`, `1F->08 B5 1A 04 05`, and the rising `11 01 01` blocks

## Recommended next captures

The highest-value next runs are now:

1. A clean side-controller reset while the compressor is definitely off
   - this should give us the best isolated `E0/E1` handshake signature
2. A clean compressor start from idle with no app changes for several minutes beforehand
   - this should improve the first `compressor_running_likely` rule
3. A deliberate fault clear or immersion-related state change, if it can be done safely
   - lower priority than the two runs above

If the goal is the shortest path to something readable like `compressor on`, this run says the answer is no longer more passive capture. It is now careful, labelled isolation of the compressor edge and the controller-reset edge.
