# Overnight Capture Summary — ebus-2026-03-21-to-2026-03-22-overnight.log

## Overview

- raw log: `/Users/jamesadams/git/gshp-ebus/logs/bus-captures/ebus-2026-03-21-to-2026-03-22-overnight.log`
- total lines: `475135`
- frame key width: `8` bytes
- unique frame keys: `127`
- parsed frame lines: `117437`
- capture duration from final stats: `20.06 h`
- final byte count: `3915334`
- final frame count: `118484`
- sync-only frames suppressed: `1383649`

## Capture Health

- framing errors: `0`
- parity errors: `0`
- FIFO overflows: `0`
- buffer-full events: `0`
- truncations: `0`
- break events: `0`
- unknown UART events: `0`

## Frame Reasons

- `sync_boundary`: `111467`
- `idle_timeout`: `5935`
- `gap_before_byte`: `35`

## Top Signatures

| Route | Type | Signature | Frames | Share | Median gap | Lengths | Vary idx |
| --- | --- | --- | ---: | ---: | ---: | --- | --- |
| `1F->15` | `B5` | `AA 1F 15 B5 24 06 02 00` | 46047 | 39.21% | 1.138s | `13,15,17,22,23,24,25,26` | `8,10,12,13,14,15,16,17,18,19,20,21,22,23,24,25,len` |
| `10->76` | `B5` | `AA 10 76 B5 11 01 01 16` | 7160 | 6.10% | 9.998s | `8,21,22` | `8,9,10,11,12,13,14,15,16,17,18,19,20,21,len` |
| `10->08` | `B5` | `AA 10 08 B5 11 01 01 89` | 7158 | 6.10% | 9.998s | `9,21,22` | `9,10,11,12,13,14,15,16,17,18,19,20,21,len` |
| `10->76` | `B5` | `AA 10 76 B5 10 09 00 00` | 6894 | 5.87% | 9.997s | `16,17,21` | `8,12,15,16,17,18,19,20,len` |
| `10->08` | `B5` | `AA 10 08 B5 10 09 00 00` | 6077 | 5.17% | 9.997s | `17,21` | `8,12,15,17,18,19,20,len` |
| `1F->08` | `B5` | `AA 1F 08 B5 1A 04 05 FF` | 5529 | 4.71% | 2.047s | `24,25` | `9,10,16,23,24,len` |
| `10->76` | `B5` | `AA 10 76 B5 12 03 0F 02` | 5000 | 4.26% | 9.998s | `10,11,21,22` | `8,9,10,11,12,13,14,15,16,17,18,19,20,21,len` |
| `1F->08` | `B5` | `AA 1F 08 B5 11 01 01 40` | 2806 | 2.39% | 24.263s | `20,21,22` | `10,11,12,13,15,16,18,19,20,21,len` |
| `1F->08` | `B5` | `AA 1F 08 B5 11 01 00 41` | 2752 | 2.34% | 24.259s | `9,20,21,22` | `9,10,11,12,13,14,15,16,17,18,19,20,21,len` |
| `1F->08` | `B5` | `AA 1F 08 B5 03 02 00 01` | 2746 | 2.34% | 24.258s | `10,19,23` | `10,11,12,13,14,15,16,17,18,19,20,21,22,len` |
| `10->76` | `B5` | `AA 10 76 B5 12 03 0F 00` | 2160 | 1.84% | 9.998s | `21,22` | `8,9,12,13,14,16,17,18,19,20,21,len` |
| `1F->15` | `B5` | `AA 1F 15 B5 24 05 03 01` | 1626 | 1.38% | 0.744s | `22,23` | `10,11,22,len` |
| `1F->15` | `B5` | `AA 1F 15 B5 24 05 03 03` | 1618 | 1.38% | 0.744s | `23` | `10,11` |
| `10->08` | `B5` | `AA 10 08 B5 10 03 05 FF` | 1194 | 1.02% | 59.992s | `11,14` | `11,12,13,len` |
| `10->08` | `B5` | `AA 10 08 B5 04 01 00 3D` | 1193 | 1.02% | 59.994s | `22` | `18,19,20` |

## Notable Observations

- The dominant family is `AA 1F 15 B5 24 06 02 00` on route `1F->15` with `46047` frames across the run.
- The main `1F->15 B5 24 06 02 00` family remained dominant and showed lengths `13,15,17,22,23,24,25,26` with variation at `8,10,12,13,14,15,16,17,18,19,20,21,22,23,24,25,len`.
- The `10->76 B5 11 01 01 16` family also repeated steadily and now varies across `8,9,10,11,12,13,14,15,16,17,18,19,20,21,len` over the longer run.
- The `10->08 B5 11 01 01 89` family repeated steadily with a median gap of `9.998 s`.
- The `1F->08 B5 1A 04 05 FF` family remained active throughout and showed limited but real variation at `9,10,16,23,24,len`.
- There are additional short or artefact-like signatures outside the long-lived tracked families, including `--`, `10->E0`, and `03->E0` routes.
- The parser found `127` unique frame keys, which is higher than the firmware family table size of `24`, so the firmware-side family count is not the full picture.

## Suggested Next Steps

- Keep the raw log untouched as the source-of-truth capture.
- Use the generated CSV to sort by cadence, count, and varying byte positions.
- Correlate `1F->15 B5 24 ...` and `1F->08 ...` families with deliberate Tado-visible changes.
- Validate `10->...` families against known Vaillant values such as outside temperature, flow temperature, and DHW state.
- Consider increasing the firmware family-capacity limit if we want the on-device summary to track every rare signature directly.

