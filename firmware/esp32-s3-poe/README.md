# ESP32-S3 PoE

This board is the planned MVP listener platform because it offers stable power and a straightforward network path for later phases.

## Intended use

- Phase 1: passive capture and logging
- Phase 2: serial-to-network bridge or lightweight telemetry publisher

## Safety constraints

- the board must only see a conditioned logic-level receive signal
- no direct electrical connection from eBUS to GPIO
- no transmit pin connected on the live system during the MVP

## Directory layout

- [current/README.md](current/README.md) - current intended production listener build
- [dev/README.md](dev/README.md) - experiments and bring-up notes
- [VERSION_NOTES.md](VERSION_NOTES.md) - milestone notes
