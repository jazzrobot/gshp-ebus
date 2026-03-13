# eBUS RX-Only Front-End PCB Layout Notes

These notes are for the first prototype PCB for the passive receive-only eBUS front-end.

## Board intent

- small interface board between the live eBUS pair and the `ESP32-S3 PoE`
- prototype-first, easy to inspect
- not yet optimised for size

## Recommended floorplan

```text
[J1 eBUS] [BR1] [R1/R2 + R3/R4 + U1 + OK1 LED side] || isolation gap || [OK1 transistor side + J2] [U3 input side]
```

If the isolated DC-DC module is a through-hole SIP part, place it close to the isolation barrier but keep its:

- input pins over the logic-side copper region
- output pins over the bus-side copper region

## Isolation barrier

Use a clear visual boundary on the PCB:

- routed slot if practical
- otherwise a no-copper, no-via keep-out corridor

Prototype target:

- `>= 6 mm` clearance
- `>= 6 mm` creepage

Do not run silkscreen labels or polygons through the barrier if that makes the split less obvious.

## Placement priorities

### Bus side

Place these close together:

- `J1`
- `BR1`
- `R1`
- `R2`
- `R3`
- `R4`
- `C1`
- `U1`
- `C2`
- `R5`
- `R6`
- `R7`
- bus-side pins of `OK1`

Goals:

- short, quiet `SENSE` trace
- short `VREF` loop
- clean local decoupling around `U1`

### Logic side

Place these close together:

- logic-side pins of `OK1`
- `R8`
- `R9`
- `J2`
- `U3` input pins
- `C4`

Goals:

- simple output path to the host ESP board
- easy probing of `EBUS_RX`

## Routing rules

- keep `SENSE` away from the optocoupler output trace
- keep `VREF` short and quiet
- keep `U1A_OUT` short to both `R5` and the optocoupler LED path
- avoid routing the bus-side analogue nodes under the optocoupler body
- avoid large copper pours attached to `SENSE`

## Copper pours

Recommended:

- one pour for `GND_EBUS`
- one pour for `GND_LOGIC`

Not allowed:

- any bridge, link, stitch, or accidental overlap between them

## Connector orientation

Recommended orientation:

- `J1` on an outside edge so the eBUS pair enters and leaves cleanly
- `J2` on the opposite edge or side to keep the logic harness away from the bus wiring

## Test access

Make sure the following are probeable without removing the board:

- `+VBUS`
- `SENSE`
- `VREF`
- `U1A_OUT`
- `+5V_EBUS`
- `GND_EBUS`
- `EBUS_RX`
- `GND_LOGIC`

Large pads are preferable to tiny vias on the first PCB.

## Mechanical notes

- allow mounting holes if the board may be fixed in an enclosure
- leave room for strain relief on both the eBUS pair and the host harness
- keep the board easy to disconnect during rollback

## Prototype preference

For the first board, optimise for:

- readability
- probing
- rework

not for compactness.
