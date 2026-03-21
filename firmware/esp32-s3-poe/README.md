# ESP32-S3 PoE

This board family is the planned MVP listener platform because it offers stable power and a straightforward network path for later phases.

The current bring-up target is the Waveshare `ESP32-S3-ETH`, wired as a passive listener with:

- `GPIO16` as the receive pin from the isolated front-end
- `3V3` on pin `36` feeding the breadboard logic rail
- `GND` on pin `38` feeding the breadboard logic return rail

## Intended use

- Phase 1: passive capture and logging
- Phase 2: serial-to-network bridge or lightweight telemetry publisher

## Safety constraints

- the board must only see a conditioned logic-level receive signal
- no direct electrical connection from eBUS to GPIO
- no transmit pin connected on the live system during the MVP

## Directory layout

- [current/README.md](current/README.md) - latest stable listener alias for quick flashing during bring-up
- [v0.2-sync-framing/README.md](v0.2-sync-framing/README.md) - first sync-aware framing iteration after live bus capture
- [v0.3-traffic-signatures/README.md](v0.3-traffic-signatures/README.md) - recurring frame-family analysis iteration
- [v0.4-frame-first/README.md](v0.4-frame-first/README.md) - quieter frame-first analysis iteration
- [v0.5-protocol-analysis/README.md](v0.5-protocol-analysis/README.md) - device-neutral route and change-tracking analysis iteration
- [v0.6-ethernet-logging/README.md](v0.6-ethernet-logging/README.md) - remote-access build with onboard Ethernet, HTTP log viewing, and overnight pull-logging support
- [dev/README.md](dev/README.md) - experiments and bring-up notes
- [VERSION_NOTES.md](VERSION_NOTES.md) - milestone notes
