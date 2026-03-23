#!/usr/bin/env python3

from __future__ import annotations

import argparse
import pathlib
import sys
import time
import urllib.error
import urllib.parse
import urllib.request


def fetch_logs(base_url: str, since_seq: int, timeout_seconds: float) -> tuple[int, list[tuple[int, str]]]:
    params = urllib.parse.urlencode({"since": str(since_seq)})
    url = f"{base_url.rstrip('/')}/logs?{params}"

    with urllib.request.urlopen(url, timeout=timeout_seconds) as response:
        body = response.read().decode("utf-8", errors="replace")

    latest_seq = since_seq
    lines: list[tuple[int, str]] = []

    for raw_line in body.splitlines():
        if raw_line.startswith("latest_seq="):
            latest_seq = int(raw_line.split("=", 1)[1] or "0")
            continue

        if not raw_line or not raw_line[0].isdigit():
            continue

        seq_text, separator, payload = raw_line.partition("\t")
        if not separator:
            continue

        seq = int(seq_text)
        if seq > since_seq:
            lines.append((seq, payload))

    return latest_seq, lines


def load_cursor(path: pathlib.Path) -> int:
    if not path.exists():
        return 0

    try:
        return int(path.read_text(encoding="utf-8").strip() or "0")
    except (OSError, ValueError):
        return 0


def save_cursor(path: pathlib.Path, value: int) -> None:
    path.write_text(f"{value}\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Poll the ESP32 eBUS HTTP log endpoint and append new lines to a file."
    )
    parser.add_argument(
        "--base-url",
        required=True,
        help="Base URL for the board, for example http://192.168.1.50",
    )
    parser.add_argument("--out", required=True, help="Output file to append captured lines to")
    parser.add_argument(
        "--poll-seconds",
        type=float,
        default=5.0,
        help="Polling interval in seconds (default: 5)",
    )
    parser.add_argument(
        "--timeout-seconds",
        type=float,
        default=10.0,
        help="HTTP timeout in seconds (default: 10)",
    )
    args = parser.parse_args()

    output_path = pathlib.Path(args.out).expanduser().resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    cursor_path = output_path.with_suffix(output_path.suffix + ".cursor")
    since_seq = load_cursor(cursor_path)

    with output_path.open("a", encoding="utf-8") as handle:
        wall_clock = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        handle.write(f"# collector_started base_url={args.base_url.rstrip('/')} since_seq={since_seq} wall_clock={wall_clock}\n")
        handle.flush()

        while True:
            try:
                latest_seq, new_lines = fetch_logs(
                    args.base_url, since_seq, args.timeout_seconds
                )

                if latest_seq < since_seq:
                    print(
                        f"collector notice: remote sequence reset from {since_seq} to {latest_seq}; refetching from zero",
                        file=sys.stderr,
                        flush=True,
                    )
                    since_seq = 0
                    save_cursor(cursor_path, since_seq)
                    latest_seq, new_lines = fetch_logs(
                        args.base_url, since_seq, args.timeout_seconds
                    )

                for seq, line in new_lines:
                    handle.write(line)
                    handle.write("\n")
                    since_seq = seq

                if new_lines:
                    handle.flush()
                    save_cursor(cursor_path, since_seq)
                    print(
                        f"appended {len(new_lines)} line(s); cursor={since_seq}",
                        file=sys.stderr,
                        flush=True,
                    )
                elif latest_seq > since_seq:
                    since_seq = latest_seq
                    save_cursor(cursor_path, since_seq)
                    print(
                        f"advanced cursor to {since_seq} without new lines in this poll",
                        file=sys.stderr,
                        flush=True,
                    )

            except urllib.error.URLError as exc:
                print(f"collector warning: {exc}", file=sys.stderr, flush=True)
            except KeyboardInterrupt:
                print("collector stopped", file=sys.stderr, flush=True)
                return 0

            time.sleep(args.poll_seconds)


if __name__ == "__main__":
    raise SystemExit(main())
