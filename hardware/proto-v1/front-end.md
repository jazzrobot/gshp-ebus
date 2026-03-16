# eBUS RX-Only Bus-Powered Front-End

## Status

**This is the Proto v1 design — the breadboard build.**

It is the simplest safe interface between the live eBUS and the ESP32-S3 PoE board.
The bus-side circuit is powered directly from the eBUS itself, so no external supply
crosses the isolation barrier. The PC817 optocoupler is the sole isolation boundary.

For the next iteration (Proto v2 / isolated DC-DC variant) see
[../proto-v2/front-end.md](../proto-v2/front-end.md).

## Implementation note

The **schematic in this document remains the source of truth**.

The first hole-by-hole breadboard layout used for bring-up proved too pin-centric and too
easy to mis-wire around the `LM393` and its `VREF` / `SENSE` nets. The next `proto-v1`
rebuild should therefore be done in a **node-first** style:

- build and label `+VBUS`, `GND_EBUS`, `+5V_EBUS`, `VREF`, `SENSE`, `U1A_OUT`, `EBUS_RX`, and `ESP_RX` as separate probeable nodes
- then run short jumpers from those nodes to the IC pins
- avoid converging multiple breadboard nets directly on the comparator pins where practical

See [bench-tests/2026-03-15-bring-up.md](bench-tests/2026-03-15-bring-up.md) for the first
bring-up record and the issues discovered in the original layout.

---

## Safety verification

The ESP32-S3 PoE is the only external power source required.

- The **logic side** of the PC817 connects to the ESP32: `3.3V → R8 → collector`,
  `emitter → GND_LOGIC`, `collector → R9 → ESP_RX`. That is all.
- The **bus side** is self-powered from the eBUS via a resistor-zener supply.
  No ESP32 power rail reaches the bus side.
- The only thing crossing the isolation barrier is light inside the PC817.
- `GND_LOGIC` (Ethernet ground) and `GND_EBUS` are never electrically connected.
- PC817 isolation rating: 5000 V.

---

## Design goals

- passive receive only
- galvanic isolation between eBUS and the ESP32 / PoE / Ethernet side via PC817
- bus-side circuit self-powered from eBUS — no external supply needed on the bus side
- low bus loading
- polarity-tolerant connection to the two eBUS wires
- clean logic-level output for a 3.3 V ESP32 RX pin
- SENSE node clamped against comparator common-mode violations

---

## High-level circuit

```text
eBUS A/B
  -> BR1 polarity bridge (4x 1N4007)
  -> R_supply + DZ2 zener supply (bus-powered +5V_EBUS rail)
  -> R1/R2 sense divider
  -> DZ1 SENSE clamp (3.3 V zener)
  -> LM393 comparator with hysteresis
  -> R7 + PC817 LED side
  -> PC817 transistor side
  -> R8/R9 -> ESP32 RX pin

ESP32 3.3 V / GND
  -> logic side only (R8 pull-up, PC817 transistor, R9, ESP_RX)
```

---

## Schematic

```text
                 eBUS INPUT AND RECTIFIER

eBUS_A o----+-----|>|-----+------------------------------o +VBUS
            |             |
            +-----|<|-----+
            |             |
eBUS_B o----+-----|>|-----+------------------------------o VBUS-
            |             |
            +-----|<|-----+

BR1 = 4x 1N4007  (polarity bridge)


                   BUS-SIDE POWER SUPPLY

+VBUS o---- R_supply (1.5k 1W preferred; 1k + 470R equivalent) ----+----o +5V_EBUS
                                               |
                                              DZ2  1N4733 (5.1 V, 1 W)
                                               |
VBUS- o--------------GND_EBUS-----------------+

C3 100 uF electrolytic from +5V_EBUS to GND_EBUS  (bulk storage)
C5 100 nF ceramic from +5V_EBUS to GND_EBUS  (local decoupling)

Note: for the current proto-v1 rebuild, use a single `R_supply = 1.5k 1W`.
      The earlier split `R_supply_A = 1k 1W` plus `R_supply_B = 470R 1/4 W`
      chain is electrically close enough to be treated as an equivalent
      archived variant. Tie `GND_EBUS` to `VBUS-`.


                   RECEIVE DETECTOR

+VBUS o---- R1 100k ----+----o SENSE
                        |
                       R2 15k
                        |
GND_EBUS o--------------+

DZ1  1N4728 (3.3 V) from SENSE to GND_EBUS  (SENSE node clamp)

+5V_EBUS o---- R3 22k ----+----o VREF
                          |
                         R4  11k
                          |
GND_EBUS o----------------+
                          |
                         C1 100 nF
                          |
GND_EBUS o----------------+


                     U1  LM393  (DIP-8)

Channel 1 used:
  Pin 3 (IN1+)  <---- SENSE
  Pin 2 (IN1-)  <---- VREF
  Pin 1 (OUT1)  ----> U1A_OUT  (open collector)

Channel 2 unused — tie both inputs to VREF, leave output unconnected:
  Pin 5 (IN2+)  tied to VREF
  Pin 6 (IN2-)  tied to VREF
  Pin 7 (OUT2)  leave unconnected

Pin 8 (VCC) --- +5V_EBUS
Pin 4 (GND) --- GND_EBUS
C2 100 nF from Pin 8 to Pin 4, placed close to U1.


                   COMPARATOR OUTPUT AND OPTO DRIVE

+5V_EBUS o---- R6 10k ----+----o U1A_OUT
                           |
+5V_EBUS o---- R7 1.2k ----|>|----+
                           OK1 LED side
                           (Pin 1 = Anode, Pin 2 = Cathode)

R5 470k from U1A_OUT back to SENSE  (hysteresis feedback)

Notes:
- U1 OUT1 is open collector; R6 is its pull-up
- When bus drops to LOW, U1 sinks; current flows R7 -> LED -> U1A_OUT
- DZ1 clamps SENSE to 3.3 V to protect U1 comparator input


                   PC817 OPTOCOUPLER

Bus side (isolated):
  Pin 1  Anode   <- from R7 junction / U1A_OUT
  Pin 2  Cathode -> U1A_OUT (sinking to GND_EBUS when bus LOW)

Logic side (ESP32 side):
  Pin 4  Collector ---+---- R8 4.7k ---- 3V3_LOGIC
                      |
                      +---- EBUS_RX
  Pin 3  Emitter  ----+---- GND_LOGIC

EBUS_RX o---- R9 100R ----o ESP_RX


                   LOGIC-SIDE CONNECTIONS TO ESP32-S3 PoE

3V3_LOGIC  <- ESP32 3.3 V rail
GND_LOGIC  <- ESP32 GND
ESP_RX     -> chosen UART RX pin on ESP32

Do NOT connect:
- any ESP TX pin to this front-end
- GND_LOGIC to eBUS wiring or GND_EBUS
- PoE / Ethernet ground to the bus side
```

---

## How the bus-side supply works

`+VBUS` (post-bridge) is always the higher potential bus rail regardless of
wiring polarity. At eBUS idle (HIGH, ~15–24 V), `R_supply` limits current into
`DZ2` which clamps the rail to 5.1 V. `C3` stores enough charge to sustain the
comparator and optocoupler LED through consecutive LOW bits without the supply
drooping significantly.

During bus LOW (~9–12 V post-bridge), the supply headroom is lower. At 9 V
post-bridge and ~8 V effective across `R_supply`:
- supply current ≈ (8 V − 5.1 V) / 1.5k ≈ 1.9 mA
- comparator draws ~0.5 mA
- LED draws ~3.0 mA (from +5V_EBUS via R7 when comparator sinks)

`C3` (100 µF) bridges the shortfall. At 2400 baud, the longest realistic
LOW run (10 consecutive bits) is ~4.2 ms. Even with `R7 = 1.2k`, the rail
droops by only about `0.06-0.07 V` over that window — negligible for
comparator and opto performance.

Power dissipation in `R_supply` (`1.5k 1W` preferred current build):
- at 20 V bus idle: about `125-130 mW`
- at 24 V theoretical max: about `210-215 mW`

The older split `1k 1W` plus `470R 1/4 W` chain remains a valid near-equivalent
archived variant if you want to mirror the first pass exactly.

---

## Signal logic

```text
Bus HIGH  -> comparator releases -> LED off -> transistor off -> EBUS_RX HIGH (R8 pull-up)
Bus LOW   -> comparator sinks    -> LED on  -> transistor on  -> EBUS_RX LOW
```

UART idle is HIGH. This matches eBUS idle (HIGH bus = idle = UART mark). Correct
polarity for 8N1 framing on the ESP32 UART RX pin.

---

## Threshold and hysteresis

VREF ≈ 5.0 V × 11k / 33k = **1.67 V**

SENSE voltage:
- at 9 V bus LOW (post-bridge ~8 V):   SENSE ≈ **1.04 V** (below VREF → comparator sinks)
- at 15 V bus HIGH (post-bridge ~14 V): SENSE ≈ **1.83 V** (above VREF → comparator releases)
- at 24 V bus HIGH (post-bridge ~23 V): SENSE ≈ **3.0 V** → clamped to 3.3 V by DZ1

Hysteresis from R5 (470k): approximately **1 V** at the bus level.
- Falling threshold (HIGH → LOW): ~13.4 V terminal voltage
- Rising threshold (LOW → HIGH): ~14.4 V terminal voltage

Window sits cleanly between the top of the LOW range (~12 V) and the
bottom of the HIGH range (~15 V).

---

## LM393 pinout reminder (DIP-8)

```text
Pin 1  OUT1     <- used (open collector, U1A_OUT)
Pin 2  IN1-     <- VREF
Pin 3  IN1+     <- SENSE
Pin 4  GND      <- GND_EBUS
Pin 5  IN2+     <- tie to VREF (unused channel)
Pin 6  IN2-     <- tie to VREF (unused channel)
Pin 7  OUT2     <- leave unconnected (unused channel)
Pin 8  VCC      <- +5V_EBUS
```

---

## PC817 pinout reminder (DIP-4)

```text
Pin 1  Anode    <- bus side, from R7 / U1A_OUT node
Pin 2  Cathode  <- bus side, to U1A_OUT (sinking node)
Pin 3  Emitter  <- logic side, to GND_LOGIC
Pin 4  Collector <- logic side, to R8 / EBUS_RX
```

---

## BOM

See [bom.csv](bom.csv) — the authoritative component list for this build.

All items are confirmed in hand or arriving before the breadboard build session.
No additional parts are required.

---

## Bench validation sequence

Before connecting to the live heating bus:

1. Do **not** connect eBUS wiring. Power only the logic side: connect ESP32 3.3 V
   and GND to `3V3_LOGIC` and `GND_LOGIC`.
2. Confirm with a multimeter that there is **no continuity** between `GND_LOGIC`
   and `GND_EBUS`. If there is, stop and find the wiring error.
3. Connect the bench PSU (current limit 50 mA) to simulate the bus.
   Use a 220R series resistor between the PSU and the eBUS input terminals.
4. Set PSU to 20 V (simulating bus HIGH / idle).
5. Confirm `+5V_EBUS` is present (multimeter: ~5.1 V).
6. Confirm `EBUS_RX` is HIGH (~3.3 V) at idle.
7. Reduce PSU to 10 V (simulating bus LOW).
8. Confirm `EBUS_RX` goes LOW (< 0.5 V).
9. Sweep PSU slowly from 9 V to 20 V and confirm the transition happens once,
   cleanly, between ~13 and ~15 V with no oscillation.
10. If a scope is available, verify the transition edges are clean with no ringing.
11. Only after all of the above pass, proceed to a live bus connection.

---

## Known differences from Proto v2

| Aspect | Proto v1 (this design) | Proto v2 (isolated DC-DC) |
|--------|------------------------|---------------------------|
| Bus-side power | Self-powered from eBUS | Isolated 5V→5V DC-DC from safe side |
| Bus loading | Slightly higher (~3 mA peak) | Minimal (~170 µA, divider only) |
| External supply needed | 3.3 V only (from ESP32) | 3.3 V + 5 V (from ESP32) |
| DC-DC module required | No | Yes (specialist order) |
| Complexity | Lower — better for breadboard | Higher — better for PCB |
| Bus interaction | Very low | Near-zero |
| Isolation barrier | PC817 optocoupler | PC817 optocoupler |
| SENSE clamp | Yes (DZ1 1N4728) | Yes (DZ1 1N4728) |
| Unused comparator handled | Yes | Yes |

Both designs use the same isolation component and meet the same safety requirements.
The DC-DC variant is preferred for the final PCB because it minimises bus interaction.
