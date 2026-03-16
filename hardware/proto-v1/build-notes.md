# Proto v1 Build Notes

Session notes, breadboard layout observations, and bring-up results go here.

## Current assessment

The first hole-by-hole breadboard layout in this file is now treated as an **archived first
attempt**, not the recommended next build.

What was proven from that first pass:

- the Waveshare `ESP32-S3-ETH` firmware boots and logs correctly
- the logic-side `GPIO16 -> ESP_RX` path works
- the bridge rectifier works
- the bus-powered `+5V_EBUS` rail comes up correctly on the live eBUS

What was not achieved:

- clean live eBUS capture

Why the layout is being retired:

- too many critical nets converged directly at the `LM393` pins
- it was too hard to inspect and probe `VREF`, `SENSE`, and `U1A_OUT` cleanly
- several wiring mistakes were easy to make and hard to spot during live debugging

Recommended next build style:

- build named, probeable nodes first: `+VBUS`, `GND_EBUS`, `+5V_EBUS`, `VREF`, `SENSE`, `U1A_OUT`, `EBUS_RX`, `ESP_RX`
- then jumper those nodes into the `LM393` and `PC817`
- keep the schematic in [front-end.md](./front-end.md) as the authority, not this archived placement map

See [bench-tests/2026-03-15-bring-up.md](bench-tests/2026-03-15-bring-up.md) for the full
record of the first session.

## Breadboard assumptions

This layout assumes a standard full-size breadboard with:

- `64` numbered rows
- `A-E` on the left, `F-J` on the right
- a centre trench between `E` and `F`
- one `+` and one `-` rail on each side

Before building, confirm the rail continuity with a multimeter. Some breadboards split
the side rails halfway down. If yours does, bridge the split with a short jumper so each
named rail below is continuous end-to-end.

## Rail assignments

Use the rails consistently from the outset:

- left `+` rail = `+5V_EBUS`
- left `-` rail = `GND_EBUS`
- right `+` rail = `3V3_LOGIC`
- right `-` rail = `GND_LOGIC`

Do **not** connect the left rails to the right rails.

## Important distinction: trench vs isolation barrier

The breadboard centre trench is only a **mechanical** feature. It is there so DIP parts can
sit across it without shorting opposite pins together.

It is **not** the galvanic isolation boundary.

In this design:

- `U1` (`LM393`) sits across the trench mechanically, but **all of its pins are on the bus side**
- `OK1` (`PC817`) also sits across the trench mechanically, but it is the **only actual isolation barrier**

So the real rule is:

- the bus-side circuit may occupy both the left and right matrix areas around `U1`
- the only signal that crosses from bus side to logic side is the optical link inside `PC817`
- the logic side begins at the PC817 transistor pins, `R8`, `R9`, and the ESP32 connection

## Layout strategy

The layout is intentionally divided into four vertical zones:

1. rows `6-18`: raw eBUS input and polarity bridge
2. rows `20-34`: bus-powered supply, comparator, threshold network
3. rows `40-41`: optocoupler boundary
4. rows `40-44` right side: logic pull-up and ESP32 RX output

This keeps the bus side visually grouped and makes the PC817 the obvious isolation
boundary.

## Node naming

- `EBUS_A`, `EBUS_B`: raw bus wires from the live eBUS pair
- `+VBUS`: rectified positive node after the bridge
- `GND_EBUS`: rectified negative node after the bridge and all bus-side return
- `+5V_EBUS`: zener-regulated bus-side supply
- `SENSE`: scaled bus voltage into the LM393
- `VREF`: comparator threshold reference
- `U1A_OUT`: LM393 channel 1 open-collector output
- `EBUS_RX`: logic-side output before the series resistor
- `ESP_RX`: output after `R9`, ready for the ESP32-S3 UART RX pin

## Archived first-pass breadboard placement

### How to read the coordinates

When a placement says:

- `e6 -> f6`

it means:

- one component leg is physically inserted into hole `E6`
- the other leg is physically inserted into hole `F6`

On a standard breadboard, all five holes on one side of a row are connected together, so:

- `F6`, `G6`, `H6`, `I6`, and `J6` are the same electrical node
- `A6`, `B6`, `C6`, `D6`, and `E6` are the same electrical node

They are **not** the same physical hole.

Example:

- `D1` anode is in `E6`
- `D1` cathode is in `F6`
- `J6` is electrically the same node as `F6`, but the diode leg is not physically inserted into `J6`

### Where the part values live

There are now three levels of detail in the proto documentation:

- [bom.csv](./bom.csv) is the authoritative list of designators, values, and part types
- [front-end.md](./front-end.md) explains what each part does in the circuit
- this file shows where each part physically goes on the breadboard

If you are building from the breadboard guide, use the table below as the quick-reference
view and keep [bom.csv](./bom.csv) open alongside it.

For the current proto-v1 rebuild:

- `R_supply` uses a single `1.5k 1W` part
- `R7` uses the `1.2k 1/2W` part
- all other resistors remain `1/4W`

The older archived first-pass layout below used a split dropper chain:

- `R_supply_A = 1k 1W`
- `R_supply_B = 470R 1/4W`

That older split chain is electrically close to the current single `1.5k 1W`
choice, but the single resistor is now the preferred build because it is simpler
and matches the part you are actually using.

Because the `1W` and `1/2W` parts have larger bodies, it is fine to shift a lead to any
electrically equivalent hole on the same row if that makes the breadboard build cleaner.
Keep the node names the same and add a short jumper if needed.

### Raw eBUS input and bridge

Use rows `6`, `10`, `14`, `18`, and `22` for the front-end entry and the bridge.

Inputs:

- `EBUS_A` input wire into `a6`
- `EBUS_B` input wire into `a10`

Bridge diodes:

- `D1` (`1N4007`): `e6 -> f6`, stripe towards `f6`
- `D2` (`1N4007`): `e10 -> f10`, stripe towards `f10`
- `D3` (`1N4007`): `b14 -> b6`, stripe towards `b6`
- `D4` (`1N4007`): `d18 -> d10`, stripe towards `d10`

Bridge jumpers:

- `a14 -> left - rail` to make row `14` part of `GND_EBUS`
- `a18 -> left - rail` to make row `18` part of `GND_EBUS`
- `j6 -> j12`
- `j10 -> h12`
- `f12 -> j22`

Result:

- row `22` right side (`f22-j22`) is the local `+VBUS` node
- left `-` rail is `GND_EBUS`

### Bus-powered supply

For the current rebuild, the preferred bus-side dropper is:

- `R_supply` (`1.5k 1W`): `+VBUS -> +5V_EBUS`

The archived row-by-row placement immediately below shows the older split
`R_supply_A + R_supply_B` variant from the first pass.

Series current limit:

- `R_supply_A` (`1k 1W`): `h22 -> h18`
- `R_supply_B` (`470R 1/4W`): `j18 -> j20`

Regulated rail:

- `i20 -> left + rail`

Zener and storage:

- `DZ2` (`1N4733`, 5.1 V): across the left rails, stripe to the left `+` rail
- `C3` (`100 uF` electrolytic): across the left rails, positive to the left `+` rail
- `C5` (`100 nF` ceramic): across the left rails near rows `20-24`

Result:

- left `+` rail is `+5V_EBUS`

### Comparator and threshold network

Place `U1` (`LM393`, DIP-8) across the centre trench with the notch facing up, using
rows `24-27`.

LM393 pin positions in this layout:

- pin `1` at row `24` left = `U1A_OUT`
- pin `2` at row `25` left = `VREF`
- pin `3` at row `26` left = `SENSE`
- pin `4` at row `27` left = `GND_EBUS`
- pin `8` at row `24` right = `+5V_EBUS`
- pin `7` at row `25` right = unused output
- pin `6` at row `26` right = unused `IN2-`
- pin `5` at row `27` right = unused `IN2+`

Supply connections:

- `j24 -> left + rail`
- `a27 -> left - rail`
- `C2` (`100 nF`) across the left rails near rows `24-27`

SENSE network:

- `i22 -> a22` jumper to bring `+VBUS` onto the left half
- `R1` (`100k`): `e22 -> b26`
- `R2` (`15k`): `c26 -> left - rail`
- `DZ1` (`1N4728`, 3.3 V): `d26 -> left - rail`, stripe towards `d26`

Comparator output and hysteresis:

- `R6` (`10k`): left `+` rail -> `c24`
- `R5` (`470k`): `d24 -> a26`

VREF network:

- `R3` (`22k`): left `+` rail -> `a30`
- `R4` (`11k 1/4W`): `c30 -> c34`
- `a34 -> left - rail`
- `C1` (`100 nF`): `b30 -> left - rail`
- `d30 -> a25` jumper for LM393 pin `2`
- `e30 -> j30` cross-trench jumper to extend `VREF` onto row `30` right
- `h30 -> j26` jumper for LM393 pin `6`
- `i30 -> j27` jumper for LM393 pin `5`

This ties the unused comparator channel inputs to `VREF` as required by the design.

## LM393 node map

Use this as the concise node-level truth for the comparator section:

- `pin 1 / OUT1` -> `R6` (`10k`) to `+5V_EBUS`, `R5` (`470k`) to `SENSE`, `PC817 pin 2`
- `pin 2 / IN1-` -> `VREF` (junction of `R3` (`22k`), `R4` (`11k`), `C1` (`100 nF`), LM393 pin 5, and LM393 pin 6)
- `pin 3 / IN1+` -> `SENSE` (junction of `R1` (`100k`), `R2` (`15k`), `DZ1` (`3.3V zener`), `R5` (`470k`), and LM393 pin 1 feedback path)
- `pin 4 / GND` -> `GND_EBUS`
- `pin 5 / IN2+` -> `VREF`
- `pin 6 / IN2-` -> `VREF`
- `pin 7 / OUT2` -> no connection
- `pin 8 / VCC` -> `+5V_EBUS`

### Optocoupler boundary

Place `OK1` (`PC817`, DIP-4) across the centre trench with the notch facing up, using
rows `40-41`.

PC817 pin positions in this layout:

- pin `1` at row `40` left = LED anode
- pin `2` at row `41` left = LED cathode
- pin `4` at row `40` right = collector
- pin `3` at row `41` right = emitter

Bus-side LED drive:

- `R7` (`1.2k 1/2W`): left `+` rail -> `c40`
- `a41 -> a24` jumper to connect PC817 cathode to `U1A_OUT`

### Logic-side pull-up and ESP32 connection

Logic-side connections:

- use a dedicated logic-side jumper: `j41 -> right - rail`
- `R8` (`4.7k`): right `+` rail -> `h40`
- `R9` (`100R`): `j40 -> j44`

Result:

- row `40` right side is `EBUS_RX`
- row `44` right side is `ESP_RX`

Connect the ESP32-S3 PoE as follows:

- ESP32 `3V3` -> right `+` rail
- ESP32 `GND` -> right `-` rail
- chosen ESP32 UART RX pin -> `j44`

Do **not** connect any ESP32 TX pin to this breadboard.

## Hole-by-hole lead map

Use this as the literal insertion guide for the physical component leads.

| Part | Value / type | Lead 1 | Lead 2 | Notes |
|------|--------------|--------|--------|-------|
| `D1` | `1N4007` diode | `E6` | `F6` | anode at `E6`, cathode at `F6`, stripe towards `F6` |
| `D2` | `1N4007` diode | `E10` | `F10` | anode at `E10`, cathode at `F10`, stripe towards `F10` |
| `D3` | `1N4007` diode | `B14` | `B6` | anode at `B14`, cathode at `B6`, stripe towards `B6` |
| `D4` | `1N4007` diode | `D18` | `D10` | anode at `D18`, cathode at `D10`, stripe towards `D10` |
| `R_supply` | `1.5k 1W` resistor | custom node-to-node | custom node-to-node | preferred current rebuild part between `+VBUS` and `+5V_EBUS` |
| `R_supply_A` | `1k 1W` resistor | `H22` | `H18` | archived upper resistor in the older `1.47k` split supply chain |
| `R_supply_B` | `470R 1/4W` resistor | `J18` | `J20` | archived lower resistor in the older `1.47k` split supply chain |
| `R1` | `100k 1/4W` resistor | `E22` | `B26` | `+VBUS` to `SENSE` |
| `R2` | `15k 1/4W` resistor | `C26` | left `-` rail | `SENSE` to `GND_EBUS` |
| `DZ1` | `1N4728 3.3V` zener | left `-` rail | `D26` | anode to `GND_EBUS`, cathode to `D26`, stripe at `D26` |
| `R3` | `22k 1/4W` resistor | left `+` rail | `A30` | `+5V_EBUS` to `VREF` divider top |
| `R4` | `11k 1/4W` resistor | `C30` | `C34` | lower leg of the `VREF` divider |
| `C1` | `100 nF` ceramic capacitor | `B30` | left `-` rail | non-polarised |
| `R5` | `470k 1/4W` resistor | `D24` | `A26` | hysteresis feedback |
| `R6` | `10k 1/4W` resistor | left `+` rail | `C24` | LM393 output pull-up |
| `R7` | `1.2k 1/2W` resistor | left `+` rail | `C40` | PC817 LED current limit |
| `R8` | `4.7k 1/4W` resistor | right `+` rail | `H40` | logic-side pull-up |
| `R9` | `100R 1/4W` resistor | `J40` | `J44` | `EBUS_RX` to `ESP_RX` |

Rail-mounted parts:

- `DZ2`: `1N4733 5.1V` zener across the left rails near rows `20-24`, stripe to the left `+` rail
- `C3`: `100 uF` electrolytic across the left rails near rows `20-24`, positive to the left `+` rail
- `C5`: `100 nF` ceramic across the left rails near rows `20-24`, non-polarised
- `C2`: `100 nF` ceramic across the left rails near rows `24-27`, non-polarised

Integrated circuits:

- `U1`: `LM393` comparator across the trench on rows `24-27`, notch up
- `OK1`: `PC817` optocoupler across the trench on rows `40-41`, notch up

Key jumpers:

- `A14 -> left - rail`
- `A18 -> left - rail`
- `J6 -> J12`
- `J10 -> H12`
- `F12 -> J22`
- `I20 -> left + rail`
- `I22 -> A22`
- `A27 -> left - rail`
- `D30 -> A25`
- `E30 -> J30`
- `H30 -> J26`
- `I30 -> J27`
- `A41 -> A24`
- `J41 -> right - rail`

## Quick visual map

This is not a pin-for-pin drawing, but it shows the intended zoning:

```text
left + rail  = +5V_EBUS                               right + rail = 3V3_LOGIC
left - rail  = GND_EBUS                               right - rail = GND_LOGIC

rows  6-22   bridge rectifier and +VBUS generation
rows 24-34   LM393, SENSE divider, VREF divider, hysteresis
rows 40-41   PC817 across trench
rows 40-44   logic pull-up and ESP_RX output

EBUS_A/B -> bridge -> +VBUS -> regulator -> LM393 -> PC817 -> R8/R9 -> ESP_RX
```

## Top-down ASCII breadboard diagram

This is a top-down view of the intended layout. It is still schematic rather than
photographic, but it follows the same row groupings and node names as the exact placement
above.

Use the exact placement and hole-by-hole tables above as the source of truth. This sketch is
only there to show the overall shape of the circuit.

```text
left + rail = +5V_EBUS                                                right + rail = 3V3_LOGIC
left - rail = GND_EBUS                                                right - rail = GND_LOGIC

          A   B   C   D   E   ||   F   G   H   I   J

row 06   EBUS_A o-----------[D1]||---------------------------o j6
         ^                     stripe ->                     |
         |                                                  |
row 10   EBUS_B o-----------[D2]||---------------------------o j10
         ^                     stripe ->                     |
         |                                                  |
row 12                                                    h12-j12
                                                            |
row 14   left - rail <- a14   b14-[D3]-b6                  |
                         stripe -> row 6                    |
                                                            |
row 18   left - rail <- a18   d18-[D4]-d10          h22-[1k]-h18-[470R]-j20
                         stripe -> row 10                    |
                                                            |
row 20                                                     i20 -> left + rail
row 22   a22 <-------------------------------------------- i22 = +VBUS bridge jumper
                      |
                      +-- e22-[R1 100k]-b26 = SENSE

row 24   U1 pin1 OUT1      LM393 across trench         U1 pin8 VCC -> left + rail
         c24-[R6 10k]-> left + rail
         a24 <----------------------------------------------------------- a41

row 25   U1 pin2 IN1- = VREF                               U1 pin7 unused
         a25 <----------------------------------------------------------- d30

row 26   U1 pin3 IN1+ = SENSE                              U1 pin6 unused IN2-
         a26-[R5 470k]-> d24                               j26 <----------- h30
         d26-[DZ1 3.3V zener]-> left - rail
         c26-[R2 15k]-> left - rail

row 27   U1 pin4 GND -> left - rail                        U1 pin5 unused IN2+
         a27 -> left - rail                                j27 <----------- i30

row 30   a30 = VREF node <-[R3 22k]- left + rail
         b30-[C1 100nF]-> left - rail
         c30-[R4 11k]-c34
row 34
         a34 -> left - rail


row 40   PC817 pin1 anode <-[R7 1.2k]- left + rail  ||  pin4 collector -> j40 = EBUS_RX
                                                      ||                    |
                                                      ||                 [R8 4.7k]
                                                      ||                    |
                                                      ||              right + rail

row 41   PC817 pin2 cathode ------------------------> a41                 j41 -> right - rail
         (to U1A_OUT / row 24)                         ||                  pin3 emitter
                                                       ||
                       real galvanic isolation boundary is inside the PC817

row 44                                                                     j44 <-[R9 100R]- j40
                                                                            |
                                                                         ESP_RX to ESP32-S3
```

Reading left to right:

- rows `6-22` turn the non-polarised eBUS pair into `+VBUS` and `GND_EBUS`
- rows `18-22` derive `+5V_EBUS` from the bus via `R_supply_A`, `R_supply_B`, and `DZ2`
- rows `24-34` detect bus HIGH versus LOW with the `LM393`
- rows `40-41` pass that state through the `PC817`
- rows `40-44` create a clean `3.3 V` logic-level `ESP_RX` signal

Important:

- the `LM393` crosses the breadboard trench mechanically, but it is still entirely on the
  bus side
- only the `PC817` crosses the galvanic isolation boundary electrically

## Recommended jumper colours

Use a fixed colour scheme and stick to it:

- red = `+5V_EBUS`
- black = `GND_EBUS`
- orange = `+VBUS`
- green = `SENSE`
- blue = `VREF`
- yellow = `U1A_OUT`
- white = `EBUS_RX` / `ESP_RX`
- bright red or pink = `3V3_LOGIC`
- grey = `GND_LOGIC`

This makes accidental left/right rail mixing much less likely.

## Sanity checks before power

Before connecting any PSU or live bus wiring:

1. Verify with a multimeter that left `-` rail and right `-` rail are **not** connected.
2. Verify with a multimeter that left `+` rail and right `+` rail are **not** connected.
3. Confirm `DZ2` stripe is on `+5V_EBUS`.
4. Confirm `DZ1` stripe is on `SENSE`.
5. Confirm `C3` polarity is correct.
6. Confirm LM393 notch orientation and pin numbering.
7. Confirm PC817 notch orientation and pin numbering.
8. Confirm row `44` right is the only node that goes to the ESP32 UART RX pin.

## Bench validation record

Date | PSU voltage | EBUS_RX state | Notes
-----|-------------|---------------|------
(fill in during build session)

## Live install record

Date | Duration | Result | Notes
-----|----------|--------|------
(fill in after first live capture)
