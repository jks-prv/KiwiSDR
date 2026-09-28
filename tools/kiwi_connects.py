#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Parse KiwiSDR-style log for user connect times.
Python 2.7 compatible version.
- Handles both (LEAVING after hh:mm:ss) and (PREEMPTED after hh:mm:ss)
- Matches on channel number only (user ID can change)
- Always reports using the user ID from the LEAVING/PREEMPTED message
- Reads from stdin if no filename given

Via Grok-AI 9/25/2026
"""

from __future__ import print_function
import sys
import re
from collections import defaultdict
from datetime import timedelta

def parse_duration(s):
    h, m, sec = map(int, s.split(':'))
    return timedelta(hours=h, minutes=m, seconds=sec)

def format_td(td):
    total = int(td.total_seconds())
    h, rem = divmod(total, 3600)
    m, s = divmod(rem, 60)
    return "{:02d}:{:02d}:{:02d}".format(h, m, s)

def process_log(input_stream, show_all=False):
    open_sessions = {}          # channel -> (arrival_lineno, arrival_user_id)
    connections = []            # list of (user_id, channel, duration)
    user_stats = defaultdict(lambda: {"count": 0, "total": timedelta(0)})

    user_re    = re.compile(r'"([^"]+)"')
    leave_re   = re.compile(r'\((?:LEAVING|PREEMPTED) after (\d+:\d+:\d+)\)')
    restart_re = re.compile(r'KiwiSDR v\d+\.\d+ -+')

    for lineno, line in enumerate(input_stream, 1):
        line = line.rstrip("\n")
        if not line:
            continue

        # ----- Server restart marker -----
        if restart_re.search(line):
            if open_sessions:
                print("Info: server restart detected on line {} – clearing {} open session(s)".format(
                    lineno, len(open_sessions)), file=sys.stderr)
                open_sessions.clear()
            continue

        fields = line.split()
        if len(fields) < 8:
            continue

        # Channel is field 8 → index 7
        try:
            channel = fields[7]
        except IndexError:
            continue

        # ----- ARRIVED -----
        if "(ARRIVED)" in line:
            user_match = user_re.search(line)
            if not user_match:
                print("Warning: no user id on ARRIVED line {}".format(lineno), file=sys.stderr)
                continue
            arrival_user = user_match.group(1)

            if channel in open_sessions:
                prev_lineno, prev_user = open_sessions[channel]
                print("Warning: duplicate ARRIVED on channel {} on line {} "
                      "(previous arrival on line {} by '{}')".format(
                          channel, lineno, prev_lineno, prev_user), file=sys.stderr)

            open_sessions[channel] = (lineno, arrival_user)

        # ----- LEAVING or PREEMPTED -----
        else:
            leave_match = leave_re.search(line)
            if leave_match:
                duration_str = leave_match.group(1)
                duration = parse_duration(duration_str)

                user_match = user_re.search(line)
                if not user_match:
                    print("Warning: no user id on LEAVING/PREEMPTED line {}".format(lineno),
                          file=sys.stderr)
                    continue
                leaving_user = user_match.group(1)

                if channel not in open_sessions:
                    print("Warning: LEAVING/PREEMPTED without matching ARRIVED on channel {} "
                          "by '{}' on line {}".format(channel, leaving_user, lineno),
                          file=sys.stderr)
                    continue

                # Record using the user ID from the LEAVING/PREEMPTED message
                connections.append((leaving_user, channel, duration))
                user_stats[leaving_user]["count"] += 1
                user_stats[leaving_user]["total"] += duration

                del open_sessions[channel]

    # Sessions that never left
    for channel, (lineno, arrival_user) in open_sessions.items():
        print("Warning: no LEAVING/PREEMPTED for channel {} (arrived on line {} as '{}')".format(
            channel, lineno, arrival_user), file=sys.stderr)

    # ---- Individual connections (only with -a) ----
    if show_all:
        print("Individual connections:")
        print("{:<30} {:<8} {}".format("User ID", "Channel", "Duration"))
        print("-" * 50)
        for user_id, channel, duration in connections:
            print("{:<30} {:<8} {}".format(user_id, channel, format_td(duration)))
        print("")

    # ---- Side-by-side summaries ----
    by_count = sorted(user_stats.items(), key=lambda x: x[1]["count"], reverse=True)
    by_time  = sorted(user_stats.items(), key=lambda x: x[1]["total"], reverse=True)

    print("\nSummary (left: by connections, right: by total time):")
    print("{:<28} {:<5} {:<10} | {:<28} {:<5} {}".format(
        "User ID", "Conn", "Total", "User ID", "Conn", "Total"))
    print("-" * 95)

    max_len = max(len(by_count), len(by_time))
    for i in range(max_len):
        # Left side (by connections)
        if i < len(by_count):
            uid1, s1 = by_count[i]
            left = "{:<28} {:<5} {:<10}".format(uid1, s1["count"], format_td(s1["total"]))
        else:
            left = "{:<45}".format("")

        # Right side (by total time)
        if i < len(by_time):
            uid2, s2 = by_time[i]
            right = "{:<28} {:<5} {}".format(uid2, s2["count"], format_td(s2["total"]))
        else:
            right = ""

        print("{} | {}".format(left, right))

if __name__ == "__main__":
    show_all = False
    args = sys.argv[1:]

    if "-a" in args:
        show_all = True
        args.remove("-a")

    if len(args) > 1:
        print("Usage: {} [-a] [logfile]".format(sys.argv[0]))
        print("  -a        Show individual connections")
        print("  logfile   Optional. Reads from stdin if omitted.")
        sys.exit(1)

    if len(args) == 1:
        # Read from file
        with open(args[0], "r") as f:
            process_log(f, show_all)
    else:
        # Read from stdin
        process_log(sys.stdin, show_all)
