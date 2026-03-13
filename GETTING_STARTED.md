# Getting Started

This guide is the shortest path from an empty repo to a safe first capture from the live eBUS.

## MVP definition

Success for the MVP means:

- the ESP32-S3 PoE listens to the bus through a safe interface
- raw frames are captured without destabilising the heat pump or Tado
- at least a small set of useful values can be identified from the capture

Home Assistant is deliberately left for the next phase.

## Hardware starting point

- ESP32-S3 PoE board
- electrically isolated eBUS interface or adapter suitable for passive listening
- temporary wiring and enclosure for bench validation
- laptop or host machine for collecting logs

## Hard rules

- Do not connect the eBUS directly to the ESP32.
- Do not power any live bus interface from the eBUS unless the interface is designed for it.
- Do not connect a transmit path to the live bus during the MVP.
- Do not start on the heating system before the interface has been bench-checked.

## Recommended path

1. Bench-check the interface.
   Confirm the eBUS side and logic side are isolated as expected, and confirm the logic-level output is suitable for the ESP32 input.
2. Build the first listener firmware.
   Configure the ESP32 for capture only, with timestamped serial logs and no transmit path.
3. Record a baseline of normal heating behaviour.
   Before touching wiring, note current temperatures, Tado state, and any visible heat pump status.
4. Install the passive tap on the live eBUS.
   Use the checklist in [docs/setup/live-ebus-install-checklist.md](docs/setup/live-ebus-install-checklist.md).
5. Capture at least 30 to 60 minutes of traffic.
   Include idle periods and a known thermostat or schedule change so the capture contains both status and control-related traffic.
6. Identify the first useful values.
   Prioritise target flow temperature, actual flow temperature, return temperature, and brine temperatures if present.

## First deliverables

- a bench note in `logs/bench-tests/`
- one or more raw captures in `logs/bus-captures/`
- a short decoding note that lists the first validated signals

## If anything looks wrong

Stop, disconnect the tap, and revert to the original wiring. The listener is not allowed to change plant behaviour even slightly.
