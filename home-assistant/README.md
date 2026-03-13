# Home Assistant

This directory is reserved for Phase 2 work.

## Phase 2 entry criteria

Do not start Home Assistant integration until Phase 1 has produced:

- stable passive captures
- a validated list of useful signals
- confidence that the listener does not affect the live system

## Likely Phase 2 options

- run `ebusd` on a nearby host and feed it from the listener
- bridge validated values into MQTT
- publish selected entities and create simple history dashboards

The transport choice should be made after the signal list is known, not before.
