# KiCad — Proto v1

KiCad schematic source for the bus-powered breadboard front-end (`proto-v1`).

## Files

- [capture-notes.md](capture-notes.md) — net names, symbol choices, and build-specific notes
- [ebus-rx-bus-powered.kicad_pro](ebus-rx-bus-powered.kicad_pro) — KiCad project file
- [ebus-rx-bus-powered.kicad_sch](ebus-rx-bus-powered.kicad_sch) — single-sheet schematic capture

## Current status

The checked-in schematic captures the current `proto-v1` circuit:

- bus-powered via `R_supply_A`, `R_supply_B`, and `DZ2`
- `R4 = 11k`
- `R7 = 1.2k`
- `PC817` logic isolation

## Notes

- the schematic has not been opened in a local KiCad install in this workspace
- footprints are intentionally left blank for most parts because this is the breadboard prototype, not the PCB design
- `BR1` is drawn as a functional bridge block even though the live breadboard build uses `4x 1N4007`
- see [../front-end.md](../front-end.md) for the circuit rationale and threshold maths
- see [../build-notes.md](../build-notes.md) for the literal breadboard placement guide
