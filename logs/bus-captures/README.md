# Bus Captures

Store raw or lightly processed capture files here.

Each capture should have enough context to be useful later:

- date and time
- whether the system was idle, heating, or changing state
- what trigger or event occurred during capture
- any known values that can help validate decoding

Current analysis notes:

- [2026-03-21-protocol-notebook.md](2026-03-21-protocol-notebook.md) - first-pass mapping of confirmed, likely, and still-unknown live families from the `v0.5` capture session
- [2026-03-21-to-2026-03-22-overnight-summary.md](2026-03-21-to-2026-03-22-overnight-summary.md) - machine-produced summary of the first long unattended capture run
- [2026-03-21-to-2026-03-22-overnight-families.csv](2026-03-21-to-2026-03-22-overnight-families.csv) - per-signature CSV extracted from the overnight run for sorting and deeper analysis
- [2026-03-21-to-2026-03-22-overnight-logical-summary-7b.md](2026-03-21-to-2026-03-22-overnight-logical-summary-7b.md) - same overnight run summarised with a `7`-byte key to reduce over-splitting
- [2026-03-21-to-2026-03-22-overnight-logical-families-7b.csv](2026-03-21-to-2026-03-22-overnight-logical-families-7b.csv) - logical-family CSV for higher-level protocol reasoning
- [2026-03-21-to-2026-03-22-overnight-time-anchors.csv](2026-03-21-to-2026-03-22-overnight-time-anchors.csv) - wall-clock anchors recovered from the confirmed on-bus Vaillant date/time broadcast
- [2026-03-22-decode-priorities.md](2026-03-22-decode-priorities.md) - current decode shortlist and working model after the overnight run
- [2026-03-22-compressor-cycle-1907-1910-analysis.md](2026-03-22-compressor-cycle-1907-1910-analysis.md) - first labelled event analysis using a user-observed compressor start and stop

Tools:

- [tools/summarise_capture.py](tools/summarise_capture.py) - parser and summariser for long capture logs
- [tools/extract_time_anchors.py](tools/extract_time_anchors.py) - extracts minute-by-minute wall-clock anchors from the confirmed `B5 16 08 00` date/time family
