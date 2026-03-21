# Firmware

This directory holds firmware notes and source code for the ESP32 listener.

## Current target

- ESP32-S3 PoE as the first listener platform

## Firmware principles

- passive by default
- no transmit path in MVP builds
- timestamped logs first, decoding second
- visible counters for framing errors, dropped bytes, and buffer overruns
- simple rollback and reflash path

## Layout

- [esp32-s3-poe/README.md](esp32-s3-poe/README.md) - board-specific notes
- [esp32-s3-poe/current/README.md](esp32-s3-poe/current/README.md) - latest stable passive listener alias
- [esp32-s3-poe/v0.2-sync-framing/README.md](esp32-s3-poe/v0.2-sync-framing/README.md) - sync-aware framing iteration after first live capture
- [esp32-s3-poe/v0.3-traffic-signatures/README.md](esp32-s3-poe/v0.3-traffic-signatures/README.md) - frame-family signature and repeat-timing analysis build
- [esp32-s3-poe/v0.4-frame-first/README.md](esp32-s3-poe/v0.4-frame-first/README.md) - quieter frame-first analysis build
- [esp32-s3-poe/v0.5-protocol-analysis/README.md](esp32-s3-poe/v0.5-protocol-analysis/README.md) - multi-device protocol-analysis build with route-like IDs and change tracking
- [esp32-s3-poe/v0.6-ethernet-logging/README.md](esp32-s3-poe/v0.6-ethernet-logging/README.md) - remote-access build with onboard Ethernet, HTTP log viewing, and overnight pull logging
