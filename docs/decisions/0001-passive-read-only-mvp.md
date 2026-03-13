# ADR 0001 - Passive, isolated, read-only MVP

- Status: accepted
- Date: 2026-03-12

## Context

The live heating system is already being controlled by Tado over eBUS. The immediate need is observability, not control.

A prototype that loads the bus incorrectly, shares grounds unsafely, or accidentally transmits could interfere with heating behaviour and create avoidable plant risk.

## Decision

The MVP will be built as a passive listener only.

The live system implementation must:

- use an interface suitable for the eBUS electrical layer
- keep the bus electrically isolated from the ESP32-S3 PoE where practical
- leave transmit disconnected or otherwise impossible to enable accidentally
- preserve Tado as the sole controller

## Consequences

Positive consequences:

- sharply lower risk to the live heating system
- clearer debugging because the project only observes
- easier rollback if anything looks wrong

Trade-offs:

- no control experiments in the MVP
- some potential interface shortcuts are deliberately ruled out
- Home Assistant integration waits until capture and decoding are stable
