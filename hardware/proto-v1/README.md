# Proto v1 — Bus-Powered Breadboard

## What this is

The first physical build. A passive eBUS receive interface built on a breadboard
using parts from general assortment kits, powered entirely from the eBUS itself.

**Purpose:** prove the capture chain end-to-end on the live bus and get real
telemetry data. This is the POC. It is not the permanent install.

## Status

Parts sourced. Ready to build.

## Contents

- [front-end.md](front-end.md) — full circuit design, schematic, threshold maths, pinouts, bench validation sequence
- [bom.csv](bom.csv) — authoritative component list; all parts confirmed in hand or arriving
- [build-notes.md](build-notes.md) — breadboard layout hints and session notes

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
