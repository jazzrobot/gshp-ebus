# eBUS RX-Only Isolated Front-End (Proto v2)

## Status

**This is the Proto v2 design — the next prototype after the breadboard proof of concept.**

It uses an isolated DC-DC converter to power the bus-side circuit from the safe side,
minimising live bus interaction to the sense divider only (~170 µA). Build this after
Proto v1 (bus-powered breadboard) has validated the capture chain on the live system.

For the breadboard build happening first, see
[../proto-v1/front-end.md](../proto-v1/front-end.md).

---

This design connects the live eBUS to the `ESP32-S3 PoE` board as a **passive listener only**.

It is intentionally designed so the ESP can **observe** the bus but cannot **drive** it.

## Design goals

- passive receive only
- galvanic isolation between eBUS and the ESP / PoE / Ethernet side
- low bus loading
- polarity-tolerant connection to the two eBUS wires
- clean logic-level output for a 3.3 V ESP32 RX pin

## Assumptions

- eBUS idle / HIGH is typically in the `15-24 V` region
- eBUS LOW is typically in the `9-12 V` region
- traffic is `2400 baud`, `8N1`
- the listener is powered from the safe side, not from the eBUS
- the bridge rectifier introduces roughly `1.2-1.4 V` of drop across the bus-side front-end

This design is based on the published open `eBUS adapter 3` receive topology, but simplified to a **receive-only** interface and powered from an **isolated DC-DC converter** instead of the eBUS.

## High-level circuit

```text
eBUS A/B
  -> BR1 polarity bridge
  -> high-value sense divider
  -> comparator with hysteresis
  -> optocoupler
  -> ESP32 RX pin

safe 5 V / GND
  -> isolated 5 V to 5 V DC-DC
  -> floating bus-side supply
```

## Proposed schematic

```text
                 eBUS INPUT AND RECTIFIER

 eBUS_A o----+-----|>|-----+------------------------------o +VBUS
             |             |
             +-----|<|-----+
             |             |
 eBUS_B o----+-----|>|-----+------------------------------o VBUS-
             |             |
             +-----|<|-----+

 BR1 = 4x 1N4007 / M7 or one bridge rectifier package


                   BUS-SIDE POWER

 safe 5V  o----+                               +----o +5V_EBUS
               |                               |
 safe GND o----+---- U3 isolated 5V->5V DC-DC +----o GND_EBUS

 safe-side input and isolated output are separate inside U3

 Tie `GND_EBUS` to `VBUS-`


                   RECEIVE DETECTOR

 +VBUS o---- R1 100k ----+----o SENSE
                         |
                        R2 15k
                         |
 GND_EBUS o--------------+-------------------------------o VBUS-

 +5V_EBUS o---- R3 22k ----+----o VREF
                           |
                          R4 11k
                           |
 GND_EBUS o----------------+ 
                           |
                          C1 100nF
                           |
 GND_EBUS o----------------+

 DZ1  1N4728 (3.3 V) from SENSE to GND_EBUS  (SENSE node clamp — protects U1 CMR)

                      U1A LM2903B / LM393B

 SENSE --------------------> +
 VREF  --------------------> -

 +5V_EBUS o---- R6 10k -----------+----o U1A_OUT
                                  |
 +5V_EBUS o---- R7 1.2k ---->|----+
                            OK1

 R5 470k from U1A_OUT back to SENSE for hysteresis

 Notes:
 - U1A output is open collector and uses `R6` as its pull-up
 - OK1 is a non-Darlington optocoupler such as CNY17-4
 - When the bus drops into the LOW region, U1A sinks current through OK1
 - DZ1 clamps SENSE to 3.3 V — prevents CMR violation on U1 input if bus transients exceed 24 V
 - local supply decoupling for `U1` and `U3` is required but omitted from the ASCII block for readability

 Unused comparator channel (U1B):
 - tie Pin 5 (IN2+) to VREF
 - tie Pin 6 (IN2-) to VREF
 - leave Pin 7 (OUT2) unconnected
 This keeps the spare channel quiet and prevents oscillation or excess supply current.


                   LOGIC SIDE OUTPUT

 3V3_LOGIC o---- R8 4.7k ----+----o EBUS_RX
                             |
                          OK1 transistor collector

 GND_LOGIC o-----------------+----o OK1 transistor emitter

 EBUS_RX o---- R9 100R ----o ESP32_RX
```

## How it works

### 1. Bridge rectifier

The eBUS is treated as non-polarised at the tap point.

`BR1` makes the internal bus-side nodes polarity-stable:

- `+VBUS` is always the more positive internal node
- `VBUS-` is always the more negative internal node

This lets the rest of the circuit ignore wiring polarity.

The cost is a diode drop. In practice, the comparator sees the **rectified internal bus voltage**, not the raw terminal voltage.

### 2. Sense divider

`R1` and `R2` scale the **post-bridge** bus voltage down to comparator-friendly levels while keeping the input inside the safe common-mode range of a `5 V` comparator.

With a `1.2 V` bridge drop assumption:

- at `9 V` bus LOW, internal `+VBUS` is about `7.8 V` and `SENSE` is about `1.02 V`
- at `12 V` bus LOW, internal `+VBUS` is about `10.8 V` and `SENSE` is about `1.41 V`
- at `15 V` bus HIGH, internal `+VBUS` is about `13.8 V` and `SENSE` is about `1.80 V`
- at `20.7 V` idle bus, internal `+VBUS` is about `19.5 V` and `SENSE` is about `2.54 V`
- at `24 V` bus HIGH, internal `+VBUS` is about `22.8 V` and `SENSE` is about `2.97 V`

That gives a useful separation between the expected LOW and HIGH regions.

### 3. Reference and threshold

`R3` and `R4` generate a local `VREF` of about `1.67 V` from the isolated `5 V` rail:

- `VREF = 5 V * 11k / (22k + 11k) = 1.67 V`

This avoids needing a dedicated precision reference and makes the threshold easy to tune with resistor values.

Without hysteresis, the threshold lands at roughly:

- internal bus threshold: about `12.8 V`
- terminal bus threshold after bridge drop: about `14.0 V`

### 4. Hysteresis

`R5` feeds a small amount of the comparator output back into `SENSE` so the threshold moves depending on the current state.

With the values shown and assuming roughly `1.2 V` bridge drop:

- falling threshold, bus moving from HIGH to LOW: about `13.4 V`
- rising threshold, bus moving from LOW to HIGH: about `14.4 V`

That gives about `1 V` of hysteresis around the transition region, which helps reject noise and slow edges.

If the threshold is too close to the live bus LOW/HIGH crossover in testing, the easiest tuning knob is the `R3/R4` divider ratio.

### 5. Optocoupler

`OK1` carries the receive state across the isolation barrier.

Bus side:

- bus HIGH -> comparator output released -> optocoupler LED off
- bus LOW -> comparator output sinks -> optocoupler LED on

Logic side:

- bus HIGH -> transistor off -> `EBUS_RX` pulled HIGH by `R8`
- bus LOW -> transistor on -> `EBUS_RX` pulled LOW

That gives a UART-friendly idle HIGH signal at the ESP RX pin.

### 6. No transmit path

This front-end cannot drive the eBUS because:

- there is no bus-side pull-down transistor
- there is no TX optocoupler
- the only direct bus connection is the high-value sensing divider

That is deliberate.

## Suggested BOM

- `BR1`: bridge rectifier, or `4x M7 / 1N4007`
- `DZ1`: `1N4728` (3.3 V, 1 W zener) — SENSE node clamp
- `R1`: `100k`, 1%
- `R2`: `15k`, 1%
- `R3`: `22k`, 1%
- `R4`: `11k`, 1%
- `R5`: `470k`, 1%
- `R6`: `10k`, 1%
- `R7`: `1.2k`, 1%
- `R8`: `4.7k`, 1%
- `R9`: `100R`
- `C1`: `100 nF`
- `C2`: `100 nF` local decoupling for `U1`
- `C3`: `10 uF` bulk capacitor on `+5V_EBUS`
- `C4`: `10 uF` bulk capacitor on `SAFE_5V`
- `OK1`: `CNY17-4`
- `U1`: `LM2903B` or `LM393B`
- `U3`: isolated `5 V -> 5 V` DC-DC converter, `1 W` class (e.g. Murata MEV1S0505SC, Traco TME 0505S, Recom R05-05)

## Why these values

The design keeps the divider current low, keeps the comparator input in range at the top of the normal eBUS HIGH region, and gives a threshold window between the expected LOW and HIGH regions even after allowing for bridge loss.

The rest of the circuit is deliberately simple:

- one comparator channel
- one optocoupler
- one isolated supply

That is enough for an MVP sniffer, without creating a transmit-capable bus interface.

## Safe-side connections to the ESP32-S3 PoE

- `3V3_LOGIC` -> ESP board `3.3 V`
- `GND_LOGIC` -> ESP board ground
- `EBUS_RX` -> internal logic-side receive node before the series resistor
- `ESP_RX` -> chosen ESP UART RX pin after `R9`
- `safe 5V` -> local `5 V` rail used to feed the isolated DC-DC converter

Do **not** connect:

- any ESP TX pin to this front-end
- `GND_LOGIC` to eBUS wiring
- PoE / Ethernet ground directly to the bus side

## Bench validation checklist

Before connecting to the live heating bus:

1. Power only the safe side and confirm the isolated `+5V_EBUS` rail is present.
2. Confirm there is **no continuity** between `GND_LOGIC` and `GND_EBUS`.
3. Feed the bus input from a current-limited bench supply through a resistor.
4. Sweep the simulated bus voltage and confirm:
   - below roughly `12 V`, `EBUS_RX` stays LOW
   - above roughly `15 V`, `EBUS_RX` stays HIGH
   - the transition happens cleanly between those regions
5. Check that transitions are clean on a scope.
6. Confirm the ESP sees idle HIGH and valid `2400 8N1` framing.

## Scope note

This is a **draft MVP circuit**, not yet a production design.

It is meant to be:

- simple
- safe
- bench-verifiable
- physically incapable of transmitting onto the live eBUS

If the receive waveform proves marginal, the next upgrade should be:

- a faster optocoupler or digital isolator on the receive path

and not a move to a full transmit-capable interface.

## Source references

- eBUS adapter 3 base board:
  [adapter.ebusd.eu/v2/base.en.html](https://adapter.ebusd.eu/v2/base.en.html)
- eBUS adapter 3 part list:
  [adapter.ebusd.eu/v2/partlist.en.html](https://adapter.ebusd.eu/v2/partlist.en.html)
- eBUS adapter 3 diagnostics:
  [adapter.ebusd.eu/v2/diagnostics.en.html](https://adapter.ebusd.eu/v2/diagnostics.en.html)
- eBUS protocol specification:
  [ab-log.ru/files/File/eBUS/Spec_Prot_12_V1_3_1_E.pdf](https://www.ab-log.ru/files/File/eBUS/Spec_Prot_12_V1_3_1_E.pdf)
