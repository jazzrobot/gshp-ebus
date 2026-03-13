# Architecture Overview

## Problem statement

The heat pump appears able to deliver more heat than the current control behaviour allows, but the live system exposes very little telemetry to confirm what is happening.

The immediate aim is therefore not to change control. It is to observe the eBUS safely enough to answer questions with real data.

## Project stance

This project is split into strict stages so the highest-risk ideas do not leak into the MVP.
Each stage must produce validated results before the next begins.

### Hardware prototype roadmap

| Proto | Design | Purpose |
|-------|--------|---------|
| **v1** | Bus-powered breadboard (PC817, LM393, zener supply) | Prove capture chain; get real data from the live bus |
| **v2** | Isolated DC-DC stripboard or PCB (CNY17-4, LM2903B, DC-DC supply) | Minimise bus interaction; preferred for permanent install |
| **v3** | Transmit-capable interface (not yet designed) | Sniff Tado HPO X command traffic; later, potentially replicate or override control |
| **PCB** | Production board via JLCPCB | Once v2 design is refined and stable — clean, permanent, mountable install |

### Software phase roadmap

1. **Phase 1 — MVP passive tap:** capture raw eBUS traffic, validate first signals
2. **Phase 2 — Home Assistant integration:** clean telemetry into HA; dashboards and history
3. **Phase 3 — Tado behaviour analysis:** correlate thermostat changes with bus traffic; identify control commands
4. **Phase 4 — Custom control research (future, separate project):** only after full bus understanding and with explicit safety review

## Live system architecture

```text
Vaillant flexoTHERM
        |
      eBUS
        |
   +----+------------------------+
   |                             |
Tado HPO X            Isolated passive tap
                                |
                          logic-level receive
                                |
                          ESP32-S3 PoE board
                                |
                    USB serial or Ethernet log stream
```

For Phase 2 the output path can extend into `ebusd`, MQTT, or a small custom bridge before Home Assistant. That choice should wait until Phase 1 produces stable captures and a validated signal list.

## Scope by phase

### Phase 1 - MVP passive tap

**In scope**

- safe parallel connection to the live eBUS
- passive capture only
- timestamped raw traffic logging
- first-pass decoding of high-value signals

**Out of scope**

- Home Assistant entities
- sending commands
- replacing Tado logic

**Exit criteria**

- no observed change in heating behaviour with the tap attached
- at least one repeatable capture session of 30 minutes or more
- at least a few validated signals extracted from the capture

### Phase 2 - Home Assistant integration

**In scope**

- choosing the transport path from listener to Home Assistant
- publishing validated sensors only
- building simple dashboards and history graphs

**Out of scope**

- automating control decisions
- writing to the bus

**Exit criteria**

- stable telemetry in Home Assistant for the agreed MVP signals
- clear traceability from raw frame to published entity

### Phase 3 - Behaviour analysis (Hardware Proto v3 required)

**In scope**

- correlating thermostat changes with bus traffic
- identifying likely control commands and setpoint changes
- understanding how Tado influences flow temperature and demand
- sniffing Tado HPO X write traffic to identify flow temperature setpoint commands

**Out of scope**

- taking over control of the plant

**Hardware dependency**

Phase 3 requires a transmit-capable interface (Proto v3) to inject test commands
and observe responses. This has not yet been designed and must be treated as a
separate design effort from the passive tap.

### Phase 4 - Custom control research (future, separate project)

This is a separate project, not a hidden extension of the MVP.

Before any control work is even considered, the project would need explicit handling for:

- compressor minimum run and off times
- frost protection
- domestic hot water cycles
- legionella protection
- pump overrun behaviour
- fault propagation and safe fallback

A production PCB (via JLCPCB or equivalent) would be the appropriate hardware
platform for this phase — not a breadboard or hand-wired prototype.

## Safety requirements

### Electrical safety

- Use an interface suitable for the eBUS electrical layer.
- Prefer galvanic isolation between the bus and the ESP32.
- Keep PoE and Ethernet ground isolated from the live bus interface.
- Treat any direct, non-isolated GPIO connection on the live system as out of bounds.

### Bus safety

- The listener must be passive for the MVP.
- Leave transmit physically disconnected or otherwise impossible to enable by mistake.
- Keep bus loading low enough that the existing devices do not see a changed electrical environment.

### Appliance safety

- Follow the appliance manual and local electrical rules when opening or wiring near the heat pump.
- Work only on the low-voltage communication side for this project.
- If there is any doubt about whether a compartment contains mains wiring, stop and verify before proceeding.

### Operational safety

- Tado remains the sole controller.
- The tap must be removable within minutes.
- Any sign of new faults, unstable behaviour, or odd timings means immediate rollback.

## Recommended implementation path

1. Choose or build an isolated passive interface.
2. Bench-validate the logic-level output before it ever touches the live plant.
3. Run RX-only listener firmware on the ESP32-S3 PoE.
4. Capture raw traffic and log it with timestamps.
5. Identify the first useful signals.
6. Only after that, select the Home Assistant ingestion path.

## First signals to target

- target flow temperature
- actual flow temperature
- return temperature
- brine in temperature
- brine out temperature
- compressor running state
- any explicit heat demand or control-related fields that can be validated
