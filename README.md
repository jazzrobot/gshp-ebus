# GSHP eBUS Tap

Passive telemetry capture for a Vaillant flexoTHERM ground source heat pump controlled by a Tado Heat Pump Optimiser X.

## Primary goal

The project is intentionally split into two phases:

1. **MVP:** tap the eBUS safely and pull out real telemetry from the live system without changing how the heat pump or Tado behaves.
2. **Phase 2:** move validated data into Home Assistant in a clean, supportable way.

## What the MVP includes

- A passive, read-only bus tap
- An electrically safe interface between eBUS and the ESP32-S3 PoE board
- Firmware that captures raw bus traffic and emits timestamped logs
- Enough decoding to prove we can extract useful signals such as flow and brine temperatures

## What the MVP explicitly does not include

- Sending any commands on the eBUS
- Replacing or overriding the Tado optimiser
- Closing the loop with Home Assistant automations
- Any change that risks compressor protection, frost protection, hot water scheduling, or other plant safety behaviour

## Safety boundaries

- **Passive only:** the live system listener must not transmit.
- **Isolated interface:** do not connect eBUS wiring directly to ESP32 GPIO, PoE ground, or Ethernet ground.
- **Low bus loading:** use a proven interface that is suitable for the eBUS electrical layer, ideally with class-0 style loading.
- **Tado remains in control:** the listener is an observer only.
- **Rollback first:** if the heat pump, Tado, or bus behaviour changes unexpectedly, remove the tap immediately and return to the stock arrangement.

## Start here

- [GETTING_STARTED.md](GETTING_STARTED.md) - shortest path to a safe MVP capture
- [docs/README.md](docs/README.md) - documentation index
- [docs/architecture/overview.md](docs/architecture/overview.md) - project scope, phases, and guard rails
- [docs/architecture/mvp-plan.md](docs/architecture/mvp-plan.md) - detailed MVP work plan and exit criteria
- [docs/setup/live-ebus-install-checklist.md](docs/setup/live-ebus-install-checklist.md) - install and rollback checklist for the live system

## Repository layout

- `docs/` - architecture, setup, decisions, and troubleshooting
- `hardware/` - electrical, mechanical, and future PCB work
- `firmware/` - ESP32 listener firmware notes and future code
- `home-assistant/` - Phase 2 integration work
- `logs/` - bench notes and bus capture records
- `tools/` - helper scripts

See [docs/repo-structure.md](docs/repo-structure.md) for the current tree.
See [CONVENTIONS.md](CONVENTIONS.md) for folder structure rules and naming conventions.
