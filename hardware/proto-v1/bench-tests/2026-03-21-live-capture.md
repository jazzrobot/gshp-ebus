# Proto v1 Bench Test Record — 2026-03-21

## Summary

This session achieved the first clean live eBUS capture with the rebuilt `proto-v1`
front-end and the Waveshare `ESP32-S3-ETH` listener on `GPIO16`.

The main conclusions were:

- the node-first rebuild fixed the earlier bring-up problems
- the passive receive chain is now working end-to-end on the live bus
- the front-end is producing clean UART traffic with no framing or parity errors
- repeated standalone `0xAA` bytes are present on the bus and appear to act as sync bytes
- repeated structured telegrams are now visible and ready for protocol interpretation
- the Waveshare board can now expose the capture remotely over Ethernet
- unattended overnight capture to a separate host on the LAN is now practical

## Setup under test

- board: Waveshare `ESP32-S3-ETH`
- firmware during first successful capture: passive listener on `GPIO16`
- later firmware refinement from the same session: sync-aware framing in [../../../firmware/esp32-s3-poe/v0.2-sync-framing/README.md](../../../firmware/esp32-s3-poe/v0.2-sync-framing/README.md)
- later firmware refinement from the same day: remote-access logging in [../../../firmware/esp32-s3-poe/v0.6-ethernet-logging/README.md](../../../firmware/esp32-s3-poe/v0.6-ethernet-logging/README.md)
- front-end type: `proto-v1` bus-powered passive receive interface from [../front-end.md](../front-end.md)
- build style: split proto-board build with a dedicated eBUS power/rectifier board and a separate comparator/opto board

## Bring-up sequence and results

### 1. Rebuilt hardware

- The original pin-centric breadboard layout was abandoned.
- The front-end was rebuilt around named nets and split into two physical sections:
  - raw eBUS input, bridge rectifier, dropper, clamp, and bus-side supply
  - `LM393`, `PC817`, `VREF`, `SENSE`, `U1A_OUT`, and logic-side output
- The bus-side dropper used a single `1.5k 1W` resistor in place of the earlier split `1k + 470R` chain.

### 2. Bus-side power and comparator bring-up

- The rebuilt front-end powered correctly from the live eBUS.
- The comparator and optocoupler stages were then connected to the ESP32 logic side.
- The bus and logic grounds remained isolated throughout.

Observed behaviour:

- immediate live traffic at the UART receiver
- no UART framing errors
- no UART parity errors
- no FIFO overflow or buffer-full faults during the observed sample

Representative stats output:

```text
[stats] up_ms=43280 bytes=1209 frames=489 trunc=0 frame_err=0 parity_err=0 fifo_ovf=0 buffer_full=0 breaks=0 unknown=0
```

### 3. First clean live capture

- The listener began receiving large volumes of repeatable traffic immediately after the live bus was attached.
- The capture contained many standalone `0xAA` bytes and recurring structured multi-byte telegrams.
- One recurring telegram shape was `26` bytes long and appeared repeatedly with only small field changes between frames.

Representative frame sample:

```text
[frame] reason=idle_timeout start_us=40041285 end_us=40150515 len=26 data=AA 1F 15 B5 24 06 02 00 00 00 9F 00 CC 00 08 00 00 9F 00 FF FF FF 7F 05 00 AA
```

Additional recurring samples from the same session included:

```text
[frame] reason=idle_timeout start_us=41178133 end_us=41287463 len=26 data=AA 1F 15 B5 24 06 02 00 00 00 A0 00 74 00 08 00 00 A0 00 FF FF FF 7F A3 00 AA
[frame] reason=idle_timeout start_us=42314837 end_us=42423981 len=26 data=AA 1F 15 B5 24 06 02 00 01 00 08 00 5C 00 08 01 01 08 00 00 00 00 00 DD 00 AA
[frame] reason=idle_timeout start_us=43452605 end_us=43561762 len=26 data=AA 1F 15 B5 24 06 02 00 01 00 05 00 1F 00 08 01 01 05 00 00 00 AC 41 B3 00 AA
```

### 4. Ethernet bring-up and remote logging

- The capture platform was then extended from USB-only observation to network access using the onboard `W5500`.
- Ethernet link came up cleanly and the board obtained a DHCP lease on the local network.
- HTTP status and log endpoints became reachable from a separate host.
- A simple cursor-based collector on `Heimdall` then began appending the live stream to disk for longer unattended capture.

Representative bring-up line:

```text
[net] got_ip ip=10.10.1.68 mac=3A:0F:02:DE:F1:F1 speed_mbps=100 duplex=full
```

Representative remote status sample:

```text
gshp-ebus status
----------------
[stats] up_ms=106254 bytes=5826 frames=173 families=23 sync_only=2024 rx_supp=5826 trunc=0 frame_err=0 parity_err=0 fifo_ovf=0 buffer_full=0 breaks=0 unknown=0
eth_connected=yes
latest_seq=617
ip=10.10.1.68
mac=3A:0F:02:DE:F1:F1
```

Representative host-side collector destination:

```text
/gshp/ebus.log
```

Observed behaviour during the first remote run:

- the board continued to capture cleanly while serving HTTP
- the collector on `Heimdall` kept up with the stream
- frame and family counts continued rising without UART errors
- the remote capture exposed additional recurring families, including slower `~60 s` groups and shorter `10->FE` telegram families

## Interpretation

- The hardware path is now validated for passive receive use.
- The repeated `0xAA` bytes are very likely bus sync characters rather than random noise.
- The repeating `26`-byte telegrams strongly suggest that the listener is now seeing real eBUS traffic, not unstable threshold chatter.
- The next bottleneck is no longer analogue bring-up; it is traffic interpretation, device attribution, and longer-run storage strategy.

## Conclusions

- `proto-v1` has now achieved its core MVP goal: clean passive receive capture from the live bus.
- The split-board, node-first rebuild is a much better physical implementation than the original breadboard layout.
- The current hardware is good enough to continue protocol observation and firmware iteration.
- The current firmware stack is now good enough for unattended remote observation over Ethernet.
- The next high-value work is decoding and correlation rather than more front-end bring-up.

## Next actions

- Use [../../../firmware/esp32-s3-poe/v0.6-ethernet-logging/README.md](../../../firmware/esp32-s3-poe/v0.6-ethernet-logging/README.md) as the current unattended-capture baseline.
- Leave the remote collector running for longer windows and mine the resulting logs for cadence, changing fields, and device-specific traffic groups.
- Start separating likely Vaillant and Tado families using route bytes, cadence, and controlled setpoint changes.
- Decide later whether the same receive chain should move to `proto-v2` or gain `TF` card logging for longer standalone retention.
