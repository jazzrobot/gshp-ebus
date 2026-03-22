#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import re
import statistics
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable


FRAME_RE = re.compile(
    r"^\[frame\] "
    r"reason=(?P<reason>\S+) "
    r"family=(?P<family>\d+) "
    r"seen=(?P<seen>\d+)"
    r"(?: delta_us=(?P<delta_us>\d+) avg_gap_us=(?P<avg_gap_us>\d+))? "
    r"route=(?P<route>.*?)(?: type=(?P<type>[0-9A-F]{2}))? "
    r"start_us=(?P<start_us>\d+) "
    r"end_us=(?P<end_us>\d+) "
    r"len=(?P<length>\d+) "
    r"sig=(?P<sig>.*?) "
    r"changed_idx=(?P<changed_idx>[^ ]+) "
    r"data=(?P<data>.+)$"
)

STATS_RE = re.compile(
    r"^\[stats\] "
    r"up_ms=(?P<up_ms>\d+) "
    r"bytes=(?P<bytes>\d+) "
    r"frames=(?P<frames>\d+) "
    r"families=(?P<families>\d+) "
    r"sync_only=(?P<sync_only>\d+) "
    r"rx_supp=(?P<rx_supp>\d+) "
    r"trunc=(?P<trunc>\d+) "
    r"frame_err=(?P<frame_err>\d+) "
    r"parity_err=(?P<parity_err>\d+) "
    r"fifo_ovf=(?P<fifo_ovf>\d+) "
    r"buffer_full=(?P<buffer_full>\d+) "
    r"breaks=(?P<breaks>\d+) "
    r"unknown=(?P<unknown>\d+)$"
)


def parse_hex_bytes(text: str) -> list[int]:
    return [int(part, 16) for part in text.split()]


def format_hex_bytes(data: Iterable[int]) -> str:
    return " ".join(f"{byte:02X}" for byte in data)


def format_indices(indices: set[int], varying_length: bool) -> str:
    if not indices and not varying_length:
        return "-"

    parts = [str(index) for index in sorted(indices)]
    if varying_length:
        parts.append("len")
    return ",".join(parts)


def format_us_as_seconds(microseconds: int | float | None) -> str:
    if microseconds is None:
        return ""
    return f"{microseconds / 1_000_000:.3f}"


@dataclass
class StatsSnapshot:
    up_ms: int
    bytes: int
    frames: int
    families: int
    sync_only: int
    rx_supp: int
    trunc: int
    frame_err: int
    parity_err: int
    fifo_ovf: int
    buffer_full: int
    breaks: int
    unknown: int


@dataclass
class SignatureSummary:
    sig: str
    route: str
    type_hex: str
    frame_count: int = 0
    reasons: Counter[str] = field(default_factory=Counter)
    lengths: Counter[int] = field(default_factory=Counter)
    first_start_us: int | None = None
    last_start_us: int | None = None
    last_end_us: int | None = None
    previous_start_us: int | None = None
    gaps_us: list[int] = field(default_factory=list)
    reference_data: list[int] | None = None
    varying_indices: set[int] = field(default_factory=set)
    varying_length: bool = False
    first_data: list[int] | None = None
    last_data: list[int] | None = None

    def ingest(self, reason: str, start_us: int, end_us: int, data: list[int]) -> None:
        self.frame_count += 1
        self.reasons[reason] += 1
        self.lengths[len(data)] += 1

        if self.first_start_us is None:
            self.first_start_us = start_us
        self.last_start_us = start_us
        self.last_end_us = end_us

        if self.previous_start_us is not None:
            self.gaps_us.append(start_us - self.previous_start_us)
        self.previous_start_us = start_us

        if self.reference_data is None:
            self.reference_data = list(data)
            self.first_data = list(data)
        else:
            compare_length = max(len(self.reference_data), len(data))
            for index in range(compare_length):
                ref_byte = self.reference_data[index] if index < len(self.reference_data) else None
                cur_byte = data[index] if index < len(data) else None
                if ref_byte != cur_byte:
                    self.varying_indices.add(index)

            if len(self.reference_data) != len(data):
                self.varying_length = True

        self.last_data = list(data)

    @property
    def dominant_reason(self) -> str:
        return self.reasons.most_common(1)[0][0] if self.reasons else ""

    @property
    def lengths_seen(self) -> str:
        return ",".join(str(length) for length in sorted(self.lengths))

    @property
    def vary_idx(self) -> str:
        return format_indices(self.varying_indices, self.varying_length)

    @property
    def min_gap_us(self) -> int | None:
        return min(self.gaps_us) if self.gaps_us else None

    @property
    def median_gap_us(self) -> float | None:
        return statistics.median(self.gaps_us) if self.gaps_us else None

    @property
    def max_gap_us(self) -> int | None:
        return max(self.gaps_us) if self.gaps_us else None

    @property
    def approx_period_bucket(self) -> str:
        gap = self.median_gap_us
        if gap is None:
            return "single"
        seconds = gap / 1_000_000
        if seconds < 1.5:
            return "~1s"
        if seconds < 5:
            return "~2s"
        if seconds < 15:
            return "~10s"
        if seconds < 40:
            return "~24s"
        if seconds < 90:
            return "~60s"
        return ">60s"


def build_key(data: list[int], key_bytes: int) -> str:
    return format_hex_bytes(data[:key_bytes])


def parse_capture(
    log_path: Path,
    key_bytes: int,
) -> tuple[dict[str, SignatureSummary], list[StatsSnapshot], Counter[str], int]:
    signatures: dict[str, SignatureSummary] = {}
    stats_snapshots: list[StatsSnapshot] = []
    reason_counts: Counter[str] = Counter()
    line_count = 0

    with log_path.open("r", encoding="utf-8", errors="replace") as handle:
        for raw_line in handle:
            line_count += 1
            line = raw_line.rstrip("\n")

            frame_match = FRAME_RE.match(line)
            if frame_match:
                route = frame_match.group("route")
                type_hex = frame_match.group("type") or ""
                data = parse_hex_bytes(frame_match.group("data"))
                start_us = int(frame_match.group("start_us"))
                end_us = int(frame_match.group("end_us"))
                reason = frame_match.group("reason")
                sig = build_key(data, key_bytes)

                summary = signatures.get(sig)
                if summary is None:
                    summary = SignatureSummary(sig=sig, route=route, type_hex=type_hex)
                    signatures[sig] = summary

                summary.ingest(reason=reason, start_us=start_us, end_us=end_us, data=data)
                reason_counts[reason] += 1
                continue

            stats_match = STATS_RE.match(line)
            if stats_match:
                stats_snapshots.append(
                    StatsSnapshot(
                        up_ms=int(stats_match.group("up_ms")),
                        bytes=int(stats_match.group("bytes")),
                        frames=int(stats_match.group("frames")),
                        families=int(stats_match.group("families")),
                        sync_only=int(stats_match.group("sync_only")),
                        rx_supp=int(stats_match.group("rx_supp")),
                        trunc=int(stats_match.group("trunc")),
                        frame_err=int(stats_match.group("frame_err")),
                        parity_err=int(stats_match.group("parity_err")),
                        fifo_ovf=int(stats_match.group("fifo_ovf")),
                        buffer_full=int(stats_match.group("buffer_full")),
                        breaks=int(stats_match.group("breaks")),
                        unknown=int(stats_match.group("unknown")),
                    )
                )

    return signatures, stats_snapshots, reason_counts, line_count


def write_csv(path: Path, summaries: list[SignatureSummary], total_frames: int) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(
            [
                "sig",
                "route",
                "type",
                "frames",
                "frame_share_pct",
                "dominant_reason",
                "period_bucket",
                "min_gap_s",
                "median_gap_s",
                "max_gap_s",
                "lengths_seen",
                "vary_idx",
                "first_start_us",
                "last_start_us",
                "first_data",
                "last_data",
            ]
        )

        for summary in summaries:
            share_pct = (summary.frame_count / total_frames * 100) if total_frames else 0.0
            writer.writerow(
                [
                    summary.sig,
                    summary.route,
                    summary.type_hex,
                    summary.frame_count,
                    f"{share_pct:.3f}",
                    summary.dominant_reason,
                    summary.approx_period_bucket,
                    format_us_as_seconds(summary.min_gap_us),
                    format_us_as_seconds(summary.median_gap_us),
                    format_us_as_seconds(summary.max_gap_us),
                    summary.lengths_seen,
                    summary.vary_idx,
                    summary.first_start_us or "",
                    summary.last_start_us or "",
                    format_hex_bytes(summary.first_data or []),
                    format_hex_bytes(summary.last_data or []),
                ]
            )


def write_markdown(
    path: Path,
    log_path: Path,
    key_bytes: int,
    line_count: int,
    summaries: list[SignatureSummary],
    stats_snapshots: list[StatsSnapshot],
    reason_counts: Counter[str],
) -> None:
    total_frames = sum(summary.frame_count for summary in summaries)
    final_stats = stats_snapshots[-1] if stats_snapshots else None
    unique_signatures = len(summaries)
    top_summaries = summaries[:15]

    lines: list[str] = []
    lines.append(f"# Overnight Capture Summary — {log_path.name}")
    lines.append("")
    lines.append("## Overview")
    lines.append("")
    lines.append(f"- raw log: `{log_path}`")
    lines.append(f"- total lines: `{line_count}`")
    lines.append(f"- frame key width: `{key_bytes}` bytes")
    lines.append(f"- unique frame keys: `{unique_signatures}`")
    lines.append(f"- parsed frame lines: `{total_frames}`")
    if final_stats:
        duration_hours = final_stats.up_ms / 3_600_000
        lines.append(f"- capture duration from final stats: `{duration_hours:.2f} h`")
        lines.append(f"- final byte count: `{final_stats.bytes}`")
        lines.append(f"- final frame count: `{final_stats.frames}`")
        lines.append(f"- sync-only frames suppressed: `{final_stats.sync_only}`")
    lines.append("")
    lines.append("## Capture Health")
    lines.append("")
    if final_stats:
        lines.append(f"- framing errors: `{final_stats.frame_err}`")
        lines.append(f"- parity errors: `{final_stats.parity_err}`")
        lines.append(f"- FIFO overflows: `{final_stats.fifo_ovf}`")
        lines.append(f"- buffer-full events: `{final_stats.buffer_full}`")
        lines.append(f"- truncations: `{final_stats.trunc}`")
        lines.append(f"- break events: `{final_stats.breaks}`")
        lines.append(f"- unknown UART events: `{final_stats.unknown}`")
    else:
        lines.append("- No `[stats]` lines were parsed.")
    lines.append("")
    lines.append("## Frame Reasons")
    lines.append("")
    for reason, count in reason_counts.most_common():
        lines.append(f"- `{reason}`: `{count}`")
    lines.append("")
    lines.append("## Top Signatures")
    lines.append("")
    lines.append("| Route | Type | Signature | Frames | Share | Median gap | Lengths | Vary idx |")
    lines.append("| --- | --- | --- | ---: | ---: | ---: | --- | --- |")
    for summary in top_summaries:
        share_pct = (summary.frame_count / total_frames * 100) if total_frames else 0.0
        median_gap = format_us_as_seconds(summary.median_gap_us) or "-"
        type_cell = summary.type_hex or "-"
        lines.append(
            f"| `{summary.route}` | `{type_cell}` | `{summary.sig}` | "
            f"{summary.frame_count} | {share_pct:.2f}% | {median_gap}s | "
            f"`{summary.lengths_seen}` | `{summary.vary_idx}` |"
        )
    lines.append("")
    lines.append("## Notable Observations")
    lines.append("")

    if top_summaries:
        dominant = top_summaries[0]
        lines.append(
            f"- The dominant family is `{dominant.sig}` on route `{dominant.route}` with "
            f"`{dominant.frame_count}` frames across the run."
        )

    for summary in summaries:
        if summary.sig == "AA 1F 15 B5 24 06 02 00":
            lines.append(
                f"- The main `1F->15 B5 24 06 02 00` family remained dominant and "
                f"showed lengths `{summary.lengths_seen}` with variation at `{summary.vary_idx}`."
            )
        if summary.sig == "AA 10 08 B5 11 01 01 89":
            lines.append(
                f"- The `10->08 B5 11 01 01 89` family repeated steadily with a median gap of "
                f"`{format_us_as_seconds(summary.median_gap_us)} s`."
            )
        if summary.sig == "AA 10 76 B5 11 01 01 16":
            lines.append(
                f"- The `10->76 B5 11 01 01 16` family also repeated steadily and "
                f"now varies across `{summary.vary_idx}` over the longer run."
            )
        if summary.sig == "AA 1F 08 B5 1A 04 05 FF":
            lines.append(
                f"- The `1F->08 B5 1A 04 05 FF` family remained active throughout and "
                f"showed limited but real variation at `{summary.vary_idx}`."
            )

    zero_family_like = [summary for summary in summaries if summary.route in {"--", "10->E0", "03->E0"}]
    if zero_family_like:
        lines.append(
            "- There are additional short or artefact-like signatures outside the long-lived tracked families, "
            "including `--`, `10->E0`, and `03->E0` routes."
        )

    if unique_signatures > 24:
        lines.append(
            f"- The parser found `{unique_signatures}` unique frame keys, which is higher than the "
            "firmware family table size of `24`, so the firmware-side family count is not the full picture."
        )
    else:
        lines.append(
            f"- The parser found `{unique_signatures}` unique frame keys in total."
        )

    lines.append("")
    lines.append("## Suggested Next Steps")
    lines.append("")
    lines.append("- Keep the raw log untouched as the source-of-truth capture.")
    lines.append("- Use the generated CSV to sort by cadence, count, and varying byte positions.")
    lines.append("- Correlate `1F->15 B5 24 ...` and `1F->08 ...` families with deliberate Tado-visible changes.")
    lines.append("- Validate `10->...` families against known Vaillant values such as outside temperature, flow temperature, and DHW state.")
    lines.append("- Consider increasing the firmware family-capacity limit if we want the on-device summary to track every rare signature directly.")
    lines.append("")

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarise a long gshp-ebus capture log.")
    parser.add_argument("log_path", type=Path, help="Path to the raw capture log")
    parser.add_argument("--summary-md", type=Path, required=True, help="Output Markdown summary path")
    parser.add_argument("--families-csv", type=Path, required=True, help="Output CSV path")
    parser.add_argument(
        "--key-bytes",
        type=int,
        default=8,
        help="Number of leading frame bytes to use as the grouping key (default: 8)",
    )
    args = parser.parse_args()

    signatures, stats_snapshots, reason_counts, line_count = parse_capture(args.log_path, args.key_bytes)

    summaries = sorted(
        signatures.values(),
        key=lambda summary: (-summary.frame_count, summary.sig),
    )

    total_frames = sum(summary.frame_count for summary in summaries)
    write_csv(args.families_csv, summaries, total_frames)
    write_markdown(
        args.summary_md,
        args.log_path,
        args.key_bytes,
        line_count,
        summaries,
        stats_snapshots,
        reason_counts,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
