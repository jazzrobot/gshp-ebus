# Proto v3 — Transmit-Capable Interface

## What this is

A future design. Not yet started.

Proto v3 will add a transmit path to the eBUS interface, enabling the system to:

- inject test commands and observe responses
- sniff Tado HPO X write traffic by stimulating known commands
- eventually replicate or override Tado control of the heat pump

## Prerequisites before this is designed

- Proto v1 and v2 must have produced stable, validated captures
- The eBUS frame structure and Tado command set must be understood from passive capture
- A specific, documented reason to transmit must exist
- Explicit safety review covering compressor protection, frost protection, hot water scheduling, and fault propagation

## This is a separate project

Proto v3 is not an extension of the MVP passive tap. It requires a new design
review, new safety boundaries, and its own set of exit criteria.

Do not begin work in this directory until the Phase 3 analysis work in
[../../docs/architecture/overview.md](../../docs/architecture/overview.md) is complete.
