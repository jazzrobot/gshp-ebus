# v0.3 Traffic Signatures

This is the second versioned listener iteration after successful live eBUS capture on the Waveshare `ESP32-S3-ETH`.

It builds on `v0.2` by making the serial log easier to interpret during early protocol analysis.

## What this build does

- configures `UART1` as a receive-only interface on `GPIO16`
- samples the eBUS listener output at `2400 8N1`
- logs each received byte with a microsecond timestamp over USB serial
- groups bytes into provisional frames using both `0xAA` sync boundaries and an idle-gap timeout
- suppresses one-byte sync-only `0xAA` frames from the frame log while still counting them in stats
- suppresses per-byte `[rx]` log spam for sync bytes that only act as frame starts
- assigns each recurring frame family a short signature based on the first `5` bytes
- tracks per-family repeat count and inter-arrival timing
- exposes UART error counters for framing, parity, FIFO overflow, and buffer-full events
- never configures a transmit pin for the eBUS interface

## Board and wiring

This build currently assumes:

- board: Waveshare `ESP32-S3-ETH`
- USB serial logging over the native USB-C port
- `GPIO16` wired to the optocoupler output through the breadboard's `ESP_RX` node
- `3V3` on pin `36` wired to the right-hand positive logic rail
- `GND` on pin `38` wired to the right-hand negative logic rail

Keep the bus side and logic side electrically separate:

- left-hand rails on the breadboard are the isolated bus-side rails
- right-hand rails are the ESP32 logic rails
- do not bond the two negative rails together

## Project layout

- [platformio.ini](platformio.ini) - build and upload configuration
- [src/main.cpp](src/main.cpp) - passive UART listener firmware

## Build

From this directory:

```bash
pio run
```

If you prefer Arduino IDE instead of PlatformIO, Waveshare's own guide uses the `ESP32S3 Dev Module` board setting for this hardware family.

## Flash

Connect the Waveshare board by USB-C, then from this directory:

```bash
pio run --target upload
```

If PlatformIO cannot find the serial port automatically, add:

```bash
pio run --target upload --upload-port /dev/tty.usbmodemXXXX
```

## Monitor

Open the USB serial monitor at `115200` baud:

```bash
pio device monitor --baud 115200
```

On boot you should see:

- a banner showing the build name and configured pin
- a line confirming that `UART1` is armed in receive-only mode
- periodic stats output even if no bus traffic is present

When the front-end is receiving traffic cleanly, you should then see:

- `[rx]` lines for individual bytes
- `[frame]` lines when a sync boundary or idle gap closes a provisional frame
- `[stats]` lines including `families`, `sync_only`, and `sync_rx_supp`
- `[family]` lines summarising repeat count and inter-arrival timing for each observed signature

## First bring-up sequence

1. Power the ESP32 board by USB-C only and confirm the banner appears.
2. Confirm the right-hand breadboard rails really are `3.3V` and `GND`.
3. Confirm there is no continuity between the right-hand `GND` rail and the isolated bus-side negative rail.
4. Connect the `ESP_RX` node to `GPIO16`.
5. Power the front-end on the bench before connecting to the live eBUS.
6. Only then connect the live bus and watch for `[rx]` and `[frame]` output.

## Bench status

Bench work to date has established that:

- the firmware flashes and boots correctly on the Waveshare `ESP32-S3-ETH`
- the listener runs on `GPIO16`
- forced-low testing on `EBUS_RX` produces `break` events as expected
- clean live eBUS capture has now been achieved with repeated structured traffic and no UART framing/parity errors
- standalone `0xAA` sync bytes are common on the bus and are now suppressed from the frame log so the structured telegrams are easier to inspect
- recurring frame families are now the main focus of firmware iteration

## Notes for this version

- `current/` is still the latest stable alias during bring-up.
- `v0.3` is an analysis-focused iteration that should be promoted to `current/` only after its log output proves more useful than `v0.2` on the live system.

The hardware-side record for the successful live capture session is in [../../../hardware/proto-v1/bench-tests/2026-03-21-live-capture.md](../../../hardware/proto-v1/bench-tests/2026-03-21-live-capture.md).

## Safety notes

- This firmware is passive only.
- `UART1` is used only for receive handling in firmware.
- There must be no physical TX wire from the ESP32 to the bus front-end.
- No eBUS write path is enabled in software.
- A quiet log is acceptable during bring-up; it is safer than guessing and forcing signal paths.
