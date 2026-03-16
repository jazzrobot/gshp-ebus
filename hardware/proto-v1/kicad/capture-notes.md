# eBUS RX-Only Bus-Powered Front-End — KiCad Capture Notes

This note describes the KiCad capture for the `proto-v1` breadboard build.

## Scope

- single-sheet schematic
- passive receive only
- bus-powered bus side
- `PC817` as the only galvanic isolation barrier
- intended to feed an `ESP32-S3 PoE` UART RX pin

## Sheet title

`eBUS_RX_ONLY_BUS_POWERED_FRONT_END`

## Design-specific choices

- `BR1` is captured as a bridge block labelled `4x1N4007`
- `R_supply_A = 1k 1W`
- `R_supply_B = 470R 1/4W`
- `R4 = 11k`
- `R7 = 1.2k 1/2W`

## Net names

Use these net labels exactly so the schematic, markdown docs, and bring-up notes stay aligned:

- `VBUS_POS`
- `GND_EBUS`
- `+5V_EBUS`
- `SENSE`
- `VREF`
- `U1A_OUT`
- `EBUS_RX`
- `ESP_RX`
- `3V3_LOGIC`
- `GND_LOGIC`

`VBUS_POS` is the same internal node referred to as `+VBUS` in the markdown design notes.

## Symbol choices

- `BR1` is a functional bridge block, not a footprint-ready part
- `U1` is shown as the used `LM393` channel only, with the actual `U1A` pin numbers
- `OK1` uses the actual `PC817` pin numbering:
  - pin `1` = anode
  - pin `2` = cathode
  - pin `4` = collector
  - pin `3` = emitter

## Important implementation note

The schematic still relies on the documented tie-off for the unused `LM393` channel:

- pin `5` -> `VREF`
- pin `6` -> `VREF`
- pin `7` left unconnected

That tie-off is documented in [../front-end.md](../front-end.md) and should be preserved in any later PCB-oriented redraw.

## Footprints

This KiCad capture leaves most footprints blank on purpose.

Reason:

- `proto-v1` is being built on a solderless breadboard
- the main job of this schematic is electrical clarity, not PCB preparation

Once the design moves beyond breadboard, footprints should be assigned in a PCB-specific pass.

## Validation status

- schematic text capture completed
- not yet opened in a local KiCad install in this workspace
- no ERC has been run here
