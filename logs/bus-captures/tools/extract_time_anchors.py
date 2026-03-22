#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import datetime as dt
import re
from pathlib import Path


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

TIME_PREFIX = [0xAA, 0x10, 0xFE, 0xB5, 0x16, 0x08, 0x00]


def parse_hex_bytes(text: str) -> list[int]:
    return [int(part, 16) for part in text.split()]


def format_hex_bytes(data: list[int]) -> str:
    return " ".join(f"{byte:02X}" for byte in data)


def bcd_to_int(value: int) -> int | None:
    high = (value >> 4) & 0x0F
    low = value & 0x0F
    if high > 9 or low > 9:
        return None
    return high * 10 + low


def decode_timestamp(data: list[int]) -> dt.datetime | None:
    if len(data) < 14 or data[:7] != TIME_PREFIX:
        return None

    second = bcd_to_int(data[7])
    minute = bcd_to_int(data[8])
    hour = bcd_to_int(data[9])
    day = bcd_to_int(data[10])
    month = bcd_to_int(data[11])
    year_suffix = bcd_to_int(data[13])

    if None in {second, minute, hour, day, month, year_suffix}:
        return None

    year = 2000 + year_suffix

    try:
        return dt.datetime(year, month, day, hour, minute, second)
    except ValueError:
        return None


def extract_anchors(log_path: Path) -> list[dict[str, str | int]]:
    anchors: list[dict[str, str | int]] = []

    with log_path.open("r", encoding="utf-8", errors="replace") as handle:
        for raw_line in handle:
            match = FRAME_RE.match(raw_line.rstrip("\n"))
            if not match:
                continue

            data = parse_hex_bytes(match.group("data"))
            timestamp = decode_timestamp(data)
            if timestamp is None:
                continue

            anchors.append(
                {
                    "start_us": int(match.group("start_us")),
                    "end_us": int(match.group("end_us")),
                    "reason": match.group("reason"),
                    "wall_clock": timestamp.isoformat(sep=" "),
                    "weekday_bus": str(data[12]),
                    "frame": format_hex_bytes(data),
                }
            )

    return anchors


def write_csv(path: Path, anchors: list[dict[str, str | int]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=["start_us", "end_us", "reason", "wall_clock", "weekday_bus", "frame"],
        )
        writer.writeheader()
        writer.writerows(anchors)


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract confirmed date/time broadcasts from a capture log.")
    parser.add_argument("log_path", type=Path, help="Path to the raw capture log")
    parser.add_argument("--anchors-csv", type=Path, required=True, help="Output CSV path")
    args = parser.parse_args()

    anchors = extract_anchors(args.log_path)
    write_csv(args.anchors_csv, anchors)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
