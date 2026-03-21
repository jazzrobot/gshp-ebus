# v0.6 Ethernet Logging

This is the fifth versioned listener iteration after successful live eBUS capture on the Waveshare `ESP32-S3-ETH`.

It builds on `v0.5` by keeping the passive frame and family analysis, while adding first-pass remote access over the board's onboard `W5500` Ethernet interface.

## What this build does

- keeps the passive `UART1` receive-only listener on `GPIO16`
- samples the eBUS listener output at `2400 8N1`
- keeps the `v0.5` frame grouping, route-like IDs, and changing-byte analysis
- brings up the onboard `W5500` Ethernet controller over SPI
- starts a small HTTP server on port `80`
- keeps a rolling in-memory log of recent serial lines for remote inspection
- exposes current status, family summaries, and recent logs over HTTP
- never configures a transmit pin for the eBUS interface

## Why this version exists

The board is powered reliably and can sit on the network, so this version aims to remove the need for a permanent USB serial session during longer capture runs.

For this phase, remote access is deliberately simple:

- the device remains passive on the eBUS
- capture logic stays local on the board
- remote clients pull current status and recent logs over HTTP

This is a good fit for the current MVP because it avoids choosing a long-term storage or ingestion format too early.

## Board and wiring

This build currently assumes:

- board: Waveshare `ESP32-S3-ETH`
- USB serial logging over the native USB-C port still works for bring-up
- `GPIO16` wired to the optocoupler output through the breadboard's `ESP_RX` node
- `3V3` on pin `36` wired to the right-hand positive logic rail
- `GND` on pin `38` wired to the right-hand negative logic rail

Ethernet assumptions:

- onboard Ethernet chip: `W5500`
- SPI wiring on the board follows the Waveshare reference pinout:
  - `SCK = GPIO13`
  - `MISO = GPIO12`
  - `MOSI = GPIO11`
  - `CS = GPIO14`
  - `RST = GPIO9`
  - `INT = GPIO10`
- IP assignment is by DHCP

Keep the bus side and logic side electrically separate:

- left-hand rails on the breadboard are the isolated bus-side rails
- right-hand rails are the ESP32 logic rails
- do not bond the two negative rails together

## Remote access

When Ethernet comes up successfully, the serial console should show a `got_ip` line with the assigned address.

The HTTP server then exposes:

- `/` - overview page in plain text
- `/status` - current listener and Ethernet status
- `/families` - current frame-family summaries
- `/logs` - recent log lines held in RAM
- `/logs?since=<seq>` - only log lines newer than the given sequence number

Example:

```text
http://<board-ip>/
http://<board-ip>/logs
http://<board-ip>/logs?since=1200
```

Each `/logs` response includes:

- `latest_seq=<n>` - the latest log line sequence known to the board
- `since_seq=<n>` - the cursor applied to this response
- one line per stored entry as `<seq><TAB><payload>`

That makes it possible to run a lightweight collector elsewhere on the network and append only new lines to disk.

## Local storage vs remote access

This version uses remote pull over Ethernet rather than writing to local storage.

That is intentional:

- the board does have a `TF` card slot, which is the better long-run local logging target
- the onboard flash could store short rotating logs, but it is not a good default for continuous capture because of flash wear and partitioning overhead
- Ethernet gives us immediate remote access with no extra hardware changes

If longer unattended retention is needed later, the next storage candidates are:

1. `TF` card logging on the board
2. UDP or TCP log shipping to a host on the LAN
3. a lightweight HTTP pull collector running elsewhere on the network

This iteration includes a simple host-side collector at [tools/http_log_collector.py](tools/http_log_collector.py).

Example overnight capture from another machine on the same LAN:

```bash
python3 tools/http_log_collector.py \
  --base-url http://<board-ip> \
  --out ~/gshp-ebus-overnight.log
```

The collector writes a small `.cursor` file beside the log so it can resume without re-appending the same lines, and it will automatically recover if the board restarts and the sequence counter resets.

## Project layout

- [platformio.ini](platformio.ini) - build and upload configuration
- [src/main.cpp](src/main.cpp) - passive UART listener plus Ethernet status server
- [tools/http_log_collector.py](tools/http_log_collector.py) - host-side HTTP pull collector for longer captures

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

- the normal passive-listener banner
- the `UART1` receive-only arm line
- Ethernet initialisation for the `W5500`
- an HTTP-server ready line
- Ethernet event lines such as `link up` and `got_ip`

When the front-end is receiving traffic cleanly, you should still see:

- `[frame]` lines when a sync boundary or idle gap closes a provisional frame
- `[stats]` lines including `families`, `sync_only`, and `rx_supp`
- `[family]` lines summarising repeat count, route-like bytes, changing positions, and inter-arrival timing for each observed signature

## First bring-up sequence

1. Power the ESP32 board by USB-C only and confirm the banner appears.
2. Confirm the right-hand breadboard rails really are `3.3V` and `GND`.
3. Confirm there is no continuity between the right-hand `GND` rail and the isolated bus-side negative rail.
4. Connect the `ESP_RX` node to `GPIO16`.
5. Connect Ethernet and confirm the board gets a DHCP address.
6. Confirm you can open `http://<board-ip>/status`.
7. Only then connect the live bus and watch both serial output and the HTTP endpoints.

## Notes for this version

- `current/` remains the latest stable alias during bring-up.
- `v0.6` is a remote-observation build and should only be promoted to `current/` after it proves stable on the live network.
- The same bus likely contains both the heat pump and the Tado controller, so `route=` remains deliberately device-neutral.
- This version does not yet push logs to another host. It only exposes them over HTTP on the board itself.
- This version can still support unattended capture by pairing the board with the included HTTP pull collector on another always-on machine.

The hardware-side record for the successful live capture session is in [../../../hardware/proto-v1/bench-tests/2026-03-21-live-capture.md](../../../hardware/proto-v1/bench-tests/2026-03-21-live-capture.md).

The first protocol notebook built from the live samples is in [../../../logs/bus-captures/2026-03-21-protocol-notebook.md](../../../logs/bus-captures/2026-03-21-protocol-notebook.md).

## Safety notes

- This firmware is passive only.
- `UART1` is used only for receive handling in firmware.
- There must be no physical TX wire from the ESP32 to the bus front-end.
- No eBUS write path is enabled in software.
- Ethernet access changes observability, not bus behaviour.
