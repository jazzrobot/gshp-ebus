# MVP Plan - Passive eBUS Tap

## Objective

Tap the live eBUS, pull out real telemetry, and prove the capture chain is safe and useful before any Home Assistant work begins.

## Success criteria

The MVP is complete when all of the following are true:

- the tap is electrically safe for the live system
- the ESP32 listener captures stable traffic without transmitting
- raw captures can be reproduced across multiple sessions
- at least a shortlist of useful signals has been identified with confidence

## Non-goals

- changing control behaviour
- publishing entities to Home Assistant
- building a polished dashboard
- replacing the Tado optimiser

## Workstreams

### 1. Hardware interface

**Goal**

Create a live-safe path from eBUS to a logic-level receive signal for the ESP32-S3 PoE.

**Tasks**

- choose an isolated interface approach for the live system
- document the exact wiring and connector plan
- keep the transmit path disconnected for the MVP
- define a simple rollback path so the tap can be removed quickly

**Done when**

- the interface wiring is documented in `hardware/`
- the live install checklist is complete
- the bench validation has shown clean logic-level output

### 2. Listener firmware

**Goal**

Produce firmware that only listens and logs.

**Tasks**

- configure UART or equivalent receive path for the bus interface
- log raw bytes with timestamps
- include framing, overflow, and error counters
- keep transmit disabled in both code and wiring

**Done when**

- the firmware boots into a passive mode by default
- it can log continuously for at least 30 minutes
- log corruption and dropped-byte rates are visible in the output

### 3. Live validation

**Goal**

Confirm the tap does not change real system behaviour.

**Tasks**

- record baseline heat pump and Tado behaviour before wiring in the tap
- connect the listener in parallel
- monitor for faults, altered timings, or display changes
- run capture sessions during both steady-state and changing demand

**Done when**

- the plant behaves normally with the tap attached
- at least one controlled event is captured, such as a setpoint change or scheduled demand change

### 4. First decoding pass

**Goal**

Move from raw traffic to evidence.

**Tasks**

- isolate recurring status frames
- identify fields that appear to track temperatures and state
- compare capture results with values visible on the heat pump where possible
- create a short validated signal list

**Done when**

- the capture has yielded at least several useful values with reasonable confidence
- each validated value can be traced back to a repeatable frame or field

## Recommended order of execution

1. Finalise the hardware interface choice.
2. Bench-test the interface.
3. Build the first listener firmware.
4. Perform the live install using the checklist.
5. Capture raw traffic.
6. Validate and document the first decoded signals.
7. Only then open the Phase 2 Home Assistant work.

## Deliverables

- updated hardware notes in `hardware/`
- firmware notes in `firmware/esp32-s3-poe/`
- a live install checklist in `docs/setup/`
- bench notes in `logs/bench-tests/`
- raw captures and summaries in `logs/bus-captures/`

## Stop conditions

Stop and roll back immediately if any of the following happen:

- Tado or the heat pump raises new faults
- the eBUS appears unstable or traffic disappears unexpectedly
- the listener firmware shows signs of transmitting or driving the line
- the interface cannot be shown to isolate the bus safely
