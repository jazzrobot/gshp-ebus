# 2026-03-21 Protocol Notebook

First-pass interpretation notes from the live `v0.5` passive capture session on `2026-03-21`.

This note is deliberately conservative. The bus contains both the Vaillant heat-pump system and the Tado HPO X, so anything that is not directly supported by an external reference stays marked as inference.

## Ground rules

- `family=` numbers from the serial log are session-local and should not be treated as stable IDs.
- `route=` is based on the request-side bytes at the start of each provisional frame. It helps us group traffic, but it does not by itself prove which device originated the returned payload.
- The `Tado HPO X` and the Vaillant plant share the same bus, so `route=10->...` and `route=1F->...` should be treated as traffic buckets first and device labels second.
- The main external reference used here is an unofficial Vaillant protocol document from `2014`, so newer devices may use traffic that is not covered there.

## Confirmed or strong matches

### `10->FE B5 16 08 00`

Status: confirmed

- This matches Vaillant `B5h 16h 00h` broadcast date/time.
- Example capture:

```text
AA 10 FE B5 16 08 00 36 15 22 21 03 06 26 32
```

- Interpreted payload:
  - block `00`
  - seconds `36`
  - minutes `15`
  - hours `22`
  - day `21`
  - month `03`
  - weekday `06`
  - year `26`

That lines up with the live test date `2026-03-21`.

### `10->FE B5 16 03 01`

Status: confirmed

- This matches Vaillant `B5h 16h 01h` broadcast outside temperature.
- Example capture:

```text
AA 10 FE B5 16 03 01 10 0A E4
```

- The `01` block and `03` data length match the reference.
- The temperature encoding appears to be the documented `DATA2b` outside-temperature format.

### `10->08 B5 10 09 ...`

Status: strong match

- This matches Vaillant `B5h 10h` operational data from room controller to burner control unit.
- Example capture:

```text
AA 10 08 B5 10 09 00 00 35 FF FF FF 06 00 00 39 00 01 01 9A 00
```

- The shape matches the document closely:
  - `SB=10h`
  - `NN=09h`
  - a `9`-byte request payload
  - short acknowledge response afterwards
- In the sample above, the lead-water target field is plausibly `0x35 = 53 C`.

### `10->76 B5 10 09 ...`

Status: strong match

- This looks like the same `B5h 10h` operational-data family, but aimed at target `76`.
- Example capture:

```text
AA 10 76 B5 10 09 00 00 00 FF FF FF 05 00 00 DD 00 01 01 9A 00
```

- The same framing pattern appears, with the same short acknowledgement structure as the `10->08` variant.

### `10->08 B5 11 01 ...`

Status: strong match

- This matches Vaillant `B5h 11h Block 01h` operational data of burner control unit to room control unit.
- Example capture:

```text
AA 10 08 B5 11 01 01 89 00 09 48 47 10 0A FF 2B 00 00 46 6C 00
```

- The structure lines up very well with the reference:
  - request `B5 11 01`
  - response length `09`
  - then `9` data bytes
- Plausible decoded fields in this sample:
  - lead-water temperature `0x48 = 72 C`
  - return-water temperature `0x47 = 71 C`
  - service-water temperature `0x2B = 43 C`
  - outside temperature encoded as the documented `DATA2b` pair `10 0A`

### `10->76 B5 11 01 ...`

Status: strong match

- This appears to be the same Vaillant `B5h 11h Block 01h` family, but for target `76`.
- Example capture:

```text
AA 10 76 B5 11 01 01 16 00 09 FF 47 10 0A FF 2B 00 01 FF 60 00
```

- The layout again matches the documented `Block 01h` response structure.

### `10->08 B5 12 02 00 64`

Status: strong match

- This matches the document's `B5h 12h` "unknown command [ping]" family.
- The reference explicitly notes:
  - `10h -> 08h`
  - `xx = 00h, yy = 64h`
  - hot-water circulation pump on

Captured example:

```text
AA 10 08 B5 12 02 00 64 AE 00 00 00 00
```

The surrounding trailing bytes in our provisional frame still need checking, but the core `10->08 B5 12 02 00 64` pattern is a strong fit.

### `10->08 B5 04 01 00 ...`

Status: likely

- This looks like Vaillant `B5h 04h Block 00h`, the date/time query family.
- Example capture:

```text
AA 10 08 B5 04 01 00 3D ...
```

- The command shape matches the reference:
  - `SB=04h`
  - `NN=01h`
  - block `00h`
- We should verify a full raw sample later, but this is already a plausible match.

## High-value unknown families

### `1F->15 B5 24 06 02 00 ...`

Status: unknown but important

- This is the dominant family in the capture.
- Typical cadence: about `1.14 s`.
- Typical length: `25` bytes, sometimes `23`.
- Typical variation:
  - `vary_idx=8,10,12,14,15,16,17,19,20,21,22,23,len`
- This is the best current candidate for a live telemetry or control-state family because it changes often and predictably.
- The `2014` Vaillant reference does not cover `B5 24`, so this may be:
  - a newer Vaillant family
  - a Tado-generated family
  - or another device-specific extension

### `1F->08 B5 03 02 00 01 ...`

Status: unknown

- Typical cadence: about `24 s`.
- Example signature:

```text
AA 1F 08 B5 03 02 00 01
```

- Not covered by the reference document we checked.

### `1F->08 B5 11 01 ...`

Status: partially understood

- These frames resemble the documented `B5 11 Block 01h` family, but with request route `1F->08` instead of `10->08`.
- Examples:
  - `AA 1F 08 B5 11 01 00 41 ...`
  - `AA 1F 08 B5 11 01 01 40 ...`
- This suggests `1F` may be another controller-like participant requesting similar operational data from the same target.
- That makes `1F` a strong candidate for either the Tado controller or a newer Vaillant supervisory module.

### `1F->08 B5 1A 04 05 FF ...`

Status: unknown

- Typical cadence in the sample: around `1.14 s` between its own repeats when active.
- Current variation appears limited to bytes `9,10`.
- Because it repeats and only a small part changes, it may represent a compact status or parameter report rather than a large telemetry block.

## Working traffic buckets

These are grouping hypotheses, not final device labels.

### Bucket A: `10->...`

Status: strongest Vaillant-style match

- `10->08 B5 10`
- `10->76 B5 10`
- `10->08 B5 11`
- `10->76 B5 11`
- `10->08 B5 12`
- `10->FE B5 16`

Reasoning:

- `10h` is listed in the reference as a main control-unit address.
- Several of these families match the reference document closely enough to decode their fields or command shape.

### Bucket B: `1F->...`

Status: unresolved

- `1F->15 B5 24`
- `1F->08 B5 03`
- `1F->08 B5 11`
- `1F->08 B5 1A`

Reasoning:

- `1Fh` does not appear in the address snippets we checked from the `2014` document.
- These families are therefore prime candidates for either:
  - newer Vaillant traffic not captured by the older document
  - or traffic generated by the Tado HPO X

## Immediate decoding priorities

1. Correlate `1F->15 B5 24 ...` with deliberate user-visible events.
   - Change Tado setpoint.
   - Toggle hot water.
   - Change heating mode.
   - Watch which of the varying byte positions move.

2. Validate `B5 10` target-temperature fields.
   - The `10->08` family appears to carry a lead-water target.
   - We should test whether changing a control target moves that field directly.

3. Compare `10->08` versus `1F->08` requests.
   - If both request similar `B5 11` data from the same target, we may be looking at two different controllers observing the same burner module.

4. Keep route labels device-neutral until correlation proves ownership.
   - `10` already looks Vaillant-like.
   - `1F` is still only a candidate for `Tado`.

## Sources

- Vaillant eBUS reference, unofficial but very useful for older Vaillant family matching:
  [scribd.com/document/607008170/vaillant-ebus-v0-5-0](https://www.scribd.com/document/607008170/vaillant-ebus-v0-5-0)
- Background article on Vaillant eBUS tapping with `ebusd` and Home Assistant:
  [medium.com/@jaba0x/making-my-vaillant-ecocompact-boiler-smart-with-ebusd-and-home-assistant-20599c39fba0](https://medium.com/@jaba0x/making-my-vaillant-ecocompact-boiler-smart-with-ebusd-and-home-assistant-20599c39fba0)
