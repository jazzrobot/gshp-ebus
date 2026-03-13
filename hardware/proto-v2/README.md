# Proto v2 — Isolated DC-DC Stripboard or PCB

## What this is

The second iteration. Same receive topology as Proto v1 but the bus-side circuit
is powered from an isolated 5V→5V DC-DC converter fed from the safe side (ESP32).
This reduces live bus interaction to the sense divider only (~170 µA).

**Purpose:** a cleaner, more permanent interface suitable for stripboard or a
dedicated PCB. The preferred design for the long-term install.

## Status

Design documented. Not yet built. Build after Proto v1 has validated the capture
chain on the live bus.

## Contents

- [front-end.md](front-end.md) — full circuit design, schematic, threshold maths, bench validation sequence
- [bom.csv](bom.csv) — component list for the DC-DC variant
- [pcb-layout-notes.md](pcb-layout-notes.md) — floorplan, routing rules, isolation slot guidance
- [kicad/](kicad/) — KiCad project files and capture notes

## Key differences from Proto v1

| Aspect | Proto v1 | Proto v2 |
|--------|----------|----------|
| Bus-side power | Self-powered from eBUS | Isolated DC-DC from safe side |
| Bus loading | ~3 mA peak | ~170 µA (divider only) |
| External supply needed | 3.3V only | 3.3V + 5V from ESP32 |
| DC-DC module | Not required | Required (specialist order) |
| Optocoupler | PC817 DIP-4 | CNY17-4 DIP-6 |
| Build platform | Breadboard | Stripboard or PCB |

Both designs use galvanic isolation and meet the same safety requirements.

## When this is done

Design ready to spin a PCB via JLCPCB. See [../../docs/architecture/overview.md](../../docs/architecture/overview.md)
for the full project roadmap.
