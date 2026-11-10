#!/usr/bin/env python3
"""Estimate link margin of the split peripherals from a captured central USB log.

The central logs the RSSI of every advertisement it sees while scanning
("split_central_device_found: [DEVICE]: <addr> ... RSSI -53") and the address it
connects to ("split_central_connected: Connected: <addr>"). Addresses that appear
in a "Connected" line are the split peripherals; their advertisement RSSI is the
sample. One boot gives about one sample per half, so power-cycle each half several
times while capturing (docs/range-test.md).

path loss = peripheral TX power (as configured when sampled) - RSSI
margin    = RSSI at a candidate TX power - receiver sensitivity

usage: split-rssi.py LOG [LOG...] [--tx 8] [--min-margin 20]
"""
import argparse
import re
import statistics
import sys

ANSI = re.compile(r"\x1b\[[0-9;]*m")
FOUND = re.compile(r"\[DEVICE\]: ([0-9A-F:]{17}) \((\w+)\).*?RSSI (-?\d+)")
CONNECTED = re.compile(r"split_central_connected: Connected: ([0-9A-F:]{17})")
SENS = {"1M": -95, "2M": -92}
CANDIDATES = [8, 4, 0, -4, -8]


def parse(paths):
    rssi, peripherals = {}, set()
    for path in paths:
        with open(path, errors="replace") as f:
            for line in f:
                line = ANSI.sub("", line)
                m = FOUND.search(line)
                if m:
                    rssi.setdefault(m.group(1), []).append(int(m.group(3)))
                    continue
                m = CONNECTED.search(line)
                if m:
                    peripherals.add(m.group(1))
    return {a: rssi.get(a, []) for a in sorted(peripherals)}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("logs", nargs="+")
    ap.add_argument("--tx", type=int, default=8, help="peripheral TX power in dBm when the log was captured (default 8)")
    ap.add_argument("--min-margin", type=int, default=20, help="flag margins below this many dB (default 20)")
    args = ap.parse_args()

    per = parse(args.logs)
    if not per:
        sys.exit("no split peripheral connection found in the log(s)")

    for addr, samples in per.items():
        print(f"\nperipheral {addr}: {len(samples)} advertisement sample(s)")
        if not samples:
            print("  connected but no advertisement RSSI was logged (directed advertising carries none); capture a boot where the half starts unpaired or re-advertises")
            continue
        lo, med, hi = min(samples), statistics.median(samples), max(samples)
        print(f"  RSSI dBm: min {lo}  median {med:g}  max {hi}")
        if len(samples) < 5:
            print(f"  only {len(samples)} sample(s): power-cycle the half more times before trusting this")
        print(f"  path loss (worst): {args.tx - lo} dB at TX {args.tx:+d} dBm")
        print("  margin dB at worst sample (flag = below %d dB):" % args.min_margin)
        print("    TX dBm | 1M PHY (-95) | 2M PHY (-92)")
        for tx in CANDIDATES:
            row = []
            for phy in ("1M", "2M"):
                margin = (lo + (tx - args.tx)) - SENS[phy]
                row.append(f"{margin:4d}{' LOW' if margin < args.min_margin else '    '}")
            print(f"    {tx:+6d} | {row[0]}     | {row[1]}")


if __name__ == "__main__":
    main()
