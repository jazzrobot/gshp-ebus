# Proto v1 — Bus-Powered Breadboard

## What this is

The first physical build. A passive eBUS receive interface built on a breadboard
using parts from general assortment kits, powered entirely from the eBUS itself.

**Purpose:** prove the capture chain end-to-end on the live bus and get real
telemetry data. This is the POC. It is not the permanent install.

## Status

First breadboard build assembled and brought up on the bench.

Current state:

- firmware boots and logs correctly on the Waveshare `ESP32-S3-ETH`
- logic-side receive path has been proven by forced-low testing
- bridge rectifier and bus-powered `+5V_EBUS` rail have been proven on the live bus
- no clean live eBUS capture has been achieved yet
- the first hole-by-hole breadboard layout is now treated as an archived first pass, not the recommended next build method

The next session should rebuild the front-end in a **node-first** layout, with clearly
separated and probeable nets for `+VBUS`, `GND_EBUS`, `+5V_EBUS`, `VREF`, `SENSE`,
`U1A_OUT`, `EBUS_RX`, and `ESP_RX`.

## Contents

- [front-end.md](front-end.md) — full circuit design, schematic, threshold maths, pinouts, bench validation sequence
- [bom.csv](bom.csv) — authoritative component list; all parts confirmed in hand or arriving
- [build-notes.md](build-notes.md) — archived first-pass breadboard layout, node notes, and rebuild guidance
- [bench-tests/2026-03-15-bring-up.md](bench-tests/2026-03-15-bring-up.md) — first live bench session record and outcomes
- [kicad/](kicad/) — KiCad schematic capture for the current bus-powered prototype

## Key design decisions

- Bus-side circuit is self-powered from the eBUS via a resistor-zener supply (1N4733, 5.1V)
- PC817 optocoupler provides galvanic isolation — the only connection between bus and ESP32 sides is light
- SENSE node is clamped by a 3.3V zener (1N4728) to protect the LM393 comparator input
- No transmit path — structurally impossible to drive the bus

## When this is done

Move to [../proto-v2/](../proto-v2/) once:

- at least 30 minutes of stable live capture is logged
- at least a handful of signals are identified with confidence
- no change in plant behaviour has been observed with the tap attached
