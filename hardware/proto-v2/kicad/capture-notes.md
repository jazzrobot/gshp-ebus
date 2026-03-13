# eBUS RX-Only Front-End - KiCad Capture Notes

This note translates the draft circuit into a practical capture plan for KiCad.

## Scope

- single-sheet schematic
- receive only
- galvanically isolated
- intended to feed an `ESP32-S3 PoE` UART RX pin

## Sheet title

`eBUS_RX_ONLY_ISOLATED_FRONT_END`

## Reference designators

- `J1` - eBUS input terminal
- `BR1` - polarity bridge
- `U3` - isolated `5 V -> 5 V` DC-DC converter
- `C3` - isolated-rail bulk capacitor
- `C4` - safe-side input bulk capacitor
- `U1` - comparator
- `R1` - bus sense top resistor
- `R2` - bus sense bottom resistor
- `R3` - `VREF` top resistor
- `R4` - `VREF` bottom resistor
- `C1` - `VREF` filter capacitor
- `R5` - hysteresis resistor
- `R6` - comparator output pull-up on bus side
- `R7` - optocoupler LED current limit
- `OK1` - optocoupler
- `C2` - comparator local decoupling
- `R8` - logic-side pull-up
- `R9` - series resistor into ESP RX
- `J2` - logic/power header to the ESP-side harness
- `TP1` - `+VBUS`
- `TP2` - `SENSE`
- `TP3` - `VREF`
- `TP4` - `U1A_OUT`
- `TP5` - `+5V_EBUS`
- `TP6` - `GND_EBUS`
- `TP7` - `EBUS_RX`
- `TP8` - `GND_LOGIC`

## Net names

Use these net labels exactly so the PCB and bring-up notes stay aligned:

- `EBUS_A`
- `EBUS_B`
- `VBUS_POS`
- `VBUS_NEG`
- `GND_EBUS`
- `+5V_EBUS`
- `SAFE_5V`
- `GND_LOGIC`
- `3V3_LOGIC`
- `SENSE`
- `VREF`
- `U1A_OUT`
- `EBUS_RX`
- `ESP_RX`

`VBUS_NEG` and `GND_EBUS` are intentionally tied together on the bus side.

`GND_EBUS` and `GND_LOGIC` must remain separate nets.

## Symbol and footprint guidance

### J1 - eBUS terminal

- Symbol: generic `Conn_01x02`
- Footprint: `TerminalBlock` 5.08 mm pitch, vertical
- Placement: board edge, bus side

### BR1 - polarity bridge

Two workable capture options:

1. Use a single bridge rectifier symbol and package.
2. Use four diodes if that is easier for sourcing or layout.

If using discrete diodes:

- symbols: `D1` to `D4`
- footprint: `SMA` or `SOD-123` depending on chosen diode

If using a bridge package:

- symbol: `Device:D_Bridge_+-AA`
- footprint: vendor-specific, verify against the chosen part

### U3 - isolated DC-DC converter

- Symbol: generic isolated DC-DC converter symbol
- Preferred part family: SIP `5 V -> 5 V`, `1 W`
- Footprint: vendor-specific, verify against the chosen module
- Placement: straddling the isolation boundary is fine electrically, but all output pins must stay on the bus-side copper region

### U1 - comparator

- Symbol: `Comparator:LM2903`
- Footprint: `Package_SO:SOIC-8_3.9x4.9mm_P1.27mm`
- Notes:
  - only one channel is used
  - tie the unused channel into a defined state, do not leave it floating

### OK1 - optocoupler

- Symbol: transistor-output optocoupler
- Preferred package: `DIP-6_W7.62mm` for easy prototyping
- Alternative: an SO-4 or SO-6 non-Darlington optocoupler if you want a smaller PCB

### Passives

- Default package: `0805`
- Use `1%` resistors on the threshold-setting parts

### J2 - logic/power header

Suggested pinout:

1. `SAFE_5V`
2. `GND_LOGIC`
3. `3V3_LOGIC`
4. `ESP_RX`

## Capture order

1. Place `J1` and `BR1`.
2. Add the bus-side supply block with `U3`, `C3`, and `C4`.
3. Add the `R1/R2` sense divider and label `SENSE`.
4. Add the `R3/R4/C1` reference block and label `VREF`.
5. Add `U1` and wire the used comparator channel.
6. Add `R6`, `R7`, `R5`, and the optocoupler LED side.
7. Add the logic-side transistor output, `R8`, `R9`, and `J2`.
8. Add test points.
9. Add explicit no-connects or biasing for the unused comparator channel.

## Comparator wiring

Used channel:

- non-inverting input -> `SENSE`
- inverting input -> `VREF`
- output -> `U1A_OUT`

Unused channel:

- tie the unused non-inverting input to `VREF`
- tie the unused inverting input to `VREF`
- leave the unused output unconnected

That keeps the spare channel quiet.

## Current project-file status

The checked-in `ebus-rx-only.kicad_sch` currently captures the **core receive path**.

It does **not yet** draw:

- the probe test points `TP1` to `TP8`
- the unused second comparator channel and its tie-off network

Those items remain documented here and should be added in the next KiCad pass.

## Decoupling

Minimum recommended decoupling:

- `C2 100 nF` from `+5V_EBUS` to `GND_EBUS`, placed close to `U1`
- `C3 10 uF` from `+5V_EBUS` to `GND_EBUS`, placed near `U3`
- `C4 10 uF` from `SAFE_5V` to `GND_LOGIC`, placed near `U3`

## PCB partitioning rules

- left or lower region: logic side
- right or upper region: bus side
- isolation barrier in between

Hard rules:

- no copper overlap between `GND_LOGIC` and `GND_EBUS`
- no stitching vias or copper pours bridging the isolation slot
- keep at least `6 mm` of creepage and clearance across the barrier for the prototype
- route `EBUS_A` and `EBUS_B` only on the bus side of the barrier
- route `SAFE_5V`, `3V3_LOGIC`, and `EBUS_RX` only on the logic side

## Test points

Bring-up is much easier if these are on the board:

- `TP1` = `+VBUS`
- `TP2` = `SENSE`
- `TP3` = `VREF`
- `TP4` = `U1A_OUT`
- `TP5` = `+5V_EBUS`
- `TP6` = `GND_EBUS`
- `TP7` = `EBUS_RX`
- `TP8` = `GND_LOGIC`

## ERC notes

- The isolation boundary means `GND_LOGIC` and `GND_EBUS` are intentionally disconnected.
- If KiCad flags this as odd, that is expected.
- Add a project note explaining the intentional split so it is not “fixed” later by accident.

## Variants

### Prototype-friendly variant

- DIP-6 optocoupler
- SIP isolated DC-DC
- 0805 passives
- large test pads

### Compact variant

- SMD optocoupler
- SMD isolated DC-DC
- tighter placement around the bus-side comparator

The prototype-friendly variant is preferred for the first board.
