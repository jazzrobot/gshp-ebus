# KiCad — Proto v2

KiCad schematic source for the isolated DC-DC front-end (Proto v2).

## Files

- [capture-notes.md](capture-notes.md) — reference designators, net names, symbol and footprint guidance, capture order
- [ebus-rx-only.kicad_pro](ebus-rx-only.kicad_pro) — KiCad project file
- [ebus-rx-only.kicad_sch](ebus-rx-only.kicad_sch) — schematic capture

## Current status

The checked-in schematic captures the core RX path.

Still to be added in the next KiCad pass:
- test points TP1–TP8
- unused second comparator channel tie-off network
- SENSE node clamp (DZ1, 1N4728)

These items are documented in [capture-notes.md](capture-notes.md) and in
[../front-end.md](../front-end.md).

## Notes

- the schematic has not been opened in a local KiCad install in this workspace
- verify all footprints against datasheets before sending for fabrication
- see [../pcb-layout-notes.md](../pcb-layout-notes.md) for floorplan and routing rules
