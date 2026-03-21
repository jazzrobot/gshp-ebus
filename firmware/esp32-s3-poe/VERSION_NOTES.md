# Version Notes

## Planned milestones

- `v0.1` - raw byte capture, timestamps, and error counters
- `v0.2` - basic frame parsing and log clean-up
- `v0.3` - recurring frame-family signatures and repeat timing
- `v0.4` - quieter frame-first logging with broader family signatures
- `v0.5` - route-like identifiers and per-family change tracking
- `v0.6` - remote-access logging over onboard Ethernet
- `v0.7` - first validated signal extraction for Phase 1 close-out

## Current status

- `v0.1` established raw byte capture and error counters on the Waveshare `ESP32-S3-ETH`
- `v0.2` now exists in [v0.2-sync-framing/](v0.2-sync-framing/README.md) and adds sync-aware frame grouping around `0xAA`
- `v0.3` now exists in [v0.3-traffic-signatures/](v0.3-traffic-signatures/README.md) and adds recurring frame-family signatures with repeat timing
- `v0.4` now exists in [v0.4-frame-first/](v0.4-frame-first/README.md) and switches to quieter frame-first logging with longer signatures
- `v0.5` now exists in [v0.5-protocol-analysis/](v0.5-protocol-analysis/README.md) and adds route-like identifiers plus per-family change tracking for multi-device bus analysis
- `v0.6` now exists in [v0.6-ethernet-logging/](v0.6-ethernet-logging/README.md) and adds onboard Ethernet plus HTTP access to current status, families, recent logs, and cursor-based log collection support
- [current/](current/README.md) is now treated as the latest stable alias for the active listener build
- current wiring assumption: `GPIO16` receive input, `3V3` on pin `36`, `GND` on pin `38`
