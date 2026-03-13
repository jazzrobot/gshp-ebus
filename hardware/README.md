# Hardware

Physical implementation of the passive eBUS tap.

## Prototype roadmap

| Stage | Directory | Status | Purpose |
|-------|-----------|--------|---------|
| **Proto v1** | [proto-v1/](proto-v1/) | **Build this first** | Bus-powered breadboard POC — get real data |
| **Proto v2** | [proto-v2/](proto-v2/) | Design complete, not yet built | Isolated DC-DC — preferred for permanent install |
| **Proto v3** | [proto-v3/](proto-v3/) | Not yet designed | Transmit-capable — future Tado sniff and control research |
| **PCB** | proto-v2/kicad/ → JLCPCB | After v2 validated | Production board for clean, permanent installation |

## Design rules (all protos)

- Logic side must be safe for 3.3 V ESP32 input
- Live bus side must not share ground with the ESP32 or PoE/Ethernet
- Transmit must be physically impossible for the MVP
- Any future transmit-capable design is a separate review (Proto v3)

## Safety boundaries

- Use galvanic isolation between eBUS wiring and the ESP32
- Keep bus loading low — high-impedance sense only
- Easy disconnect: the tap must be removable within minutes
- Rollback first: any sign of changed plant behaviour means immediate removal

## Shared resources

- [mechanical/](mechanical/) — enclosure, mounting, and cable management notes

## Start building

Go to [proto-v1/](proto-v1/) and read the README.
