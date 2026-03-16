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
- [esp32-s3-poe/current/README.md](esp32-s3-poe/current/README.md) - current passive listener implementation
