# Overnight Capture Summary — ebus-2026-03-22-to-2026-03-23-overnight.log

## Overview

- raw log: `logs/bus-captures/ebus-2026-03-22-to-2026-03-23-overnight.log.gz`
- total lines: `635646`
- frame key width: `8` bytes
- unique frame keys: `141`
- parsed frame lines: `157072`
- capture duration from final stats: `46.74 h`
- final byte count: `9121222`
- final frame count: `275970`
- sync-only frames suppressed: `3223664`

## Capture Health

- framing errors: `0`
- parity errors: `0`
- FIFO overflows: `0`
- buffer-full events: `0`
- truncations: `0`
- break events: `0`
- unknown UART events: `0`

## Frame Reasons

- `sync_boundary`: `149133`
- `idle_timeout`: `7884`
- `gap_before_byte`: `55`

## Top Signatures

| Route | Type | Signature | Frames | Share | Median gap | Lengths | Vary idx |
| --- | --- | --- | ---: | ---: | ---: | --- | --- |
| `1F->15` | `B5` | `AA 1F 15 B5 24 06 02 00` | 61570 | 39.20% | 1.138s | `13,17,21,22,23,24,25,26` | `8,10,12,13,14,15,16,17,18,19,20,21,22,23,24,25,len` |
| `10->76` | `B5` | `AA 10 76 B5 10 09 00 00` | 9585 | 6.10% | 9.997s | `16,21` | `12,15,16,17,18,19,20,len` |
| `10->76` | `B5` | `AA 10 76 B5 11 01 01 16` | 9583 | 6.10% | 9.998s | `9,21,22` | `9,10,11,12,13,14,15,16,17,18,19,20,21,len` |
| `10->08` | `B5` | `AA 10 08 B5 11 01 01 89` | 9579 | 6.10% | 9.998s | `8,9,21,22` | `8,9,10,11,12,13,14,15,16,17,18,19,20,21,len` |
| `10->08` | `B5` | `AA 10 08 B5 10 09 00 00` | 9377 | 5.97% | 9.997s | `16,17,21` | `8,12,15,16,17,18,19,20,len` |
| `1F->08` | `B5` | `AA 1F 08 B5 1A 04 05 FF` | 7407 | 4.72% | 2.610s | `12,24,25` | `9,10,12,13,14,15,16,17,18,19,20,21,22,23,24,len` |
| `10->76` | `B5` | `AA 10 76 B5 12 03 0F 02` | 6703 | 4.27% | 9.998s | `11,21,22` | `11,12,13,14,15,16,17,18,19,20,21,len` |
| `1F->08` | `B5` | `AA 1F 08 B5 11 01 01 40` | 3757 | 2.39% | 24.262s | `9,20,21,22` | `9,10,11,12,13,14,15,16,17,18,19,20,21,len` |
| `1F->08` | `B5` | `AA 1F 08 B5 03 02 00 01` | 3686 | 2.35% | 24.258s | `10,22,23` | `10,11,12,13,14,15,16,17,18,19,20,21,22,len` |
| `1F->08` | `B5` | `AA 1F 08 B5 11 01 00 41` | 3683 | 2.34% | 24.258s | `18,20,21,22` | `10,11,12,13,14,15,16,17,18,19,20,21,len` |
| `10->76` | `B5` | `AA 10 76 B5 12 03 0F 00` | 2880 | 1.83% | 9.998s | `11,21,22` | `8,9,11,12,13,14,15,16,17,18,19,20,21,len` |
| `1F->15` | `B5` | `AA 1F 15 B5 24 05 03 03` | 2176 | 1.39% | 0.744s | `22,23` | `10,11,15,16,21,22,len` |
| `1F->15` | `B5` | `AA 1F 15 B5 24 05 03 01` | 2167 | 1.38% | 0.744s | `22,23` | `10,11,22,len` |
| `10->08` | `B5` | `AA 10 08 B5 04 01 00 3D` | 1597 | 1.02% | 59.994s | `22` | `18,19,20` |
| `10->FE` | `B5` | `AA 10 FE B5 05 02 5C 00` | 1597 | 1.02% | 59.993s | `9` | `-` |

## Notable Observations

- The dominant family is `AA 1F 15 B5 24 06 02 00` on route `1F->15` with `61570` frames across the run.
- The main `1F->15 B5 24 06 02 00` family remained dominant and showed lengths `13,17,21,22,23,24,25,26` with variation at `8,10,12,13,14,15,16,17,18,19,20,21,22,23,24,25,len`.
- The `10->76 B5 11 01 01 16` family also repeated steadily and now varies across `9,10,11,12,13,14,15,16,17,18,19,20,21,len` over the longer run.
- The `10->08 B5 11 01 01 89` family repeated steadily with a median gap of `9.998 s`.
- The `1F->08 B5 1A 04 05 FF` family remained active throughout and showed limited but real variation at `9,10,12,13,14,15,16,17,18,19,20,21,22,23,24,len`.
- There are additional short or artefact-like signatures outside the long-lived tracked families, including `--`, `10->E0`, and `03->E0` routes.
- The parser found `141` unique frame keys, which is higher than the firmware family table size of `24`, so the firmware-side family count is not the full picture.

## Suggested Next Steps

- Keep the raw log untouched as the source-of-truth capture.
- Use the generated CSV to sort by cadence, count, and varying byte positions.
- Correlate `1F->15 B5 24 ...` and `1F->08 ...` families with deliberate Tado-visible changes.
- Validate `10->...` families against known Vaillant values such as outside temperature, flow temperature, and DHW state.
- Consider increasing the firmware family-capacity limit if we want the on-device summary to track every rare signature directly.
