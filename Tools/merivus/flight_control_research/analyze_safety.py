#!/usr/bin/env python3
"""Check AFCR zero-output boundaries against ULog mode and diagnostic samples."""

import argparse
import bisect
import json
import math
from pathlib import Path

from pyulog import ULog

import evaluate


EXPECTED_MODE = {
    "position_to_altitude": 1,
    "position_to_stabilized": 15,
    "stabilized_to_position": 2,
    "rtl": 5,
    "landing": 18,
    "takeoff": 17,
}


def name_at(data, index):
    return bytes(int(data[f"name[{byte}]"][index]) for byte in range(10)).split(b"\0", 1)[0]


def score_boundary(ulog, event):
    log = ULog(str(ulog))
    status = evaluate.dataset(log, "vehicle_status")
    debug = evaluate.dataset(log, "debug_vect")
    start = int(event["start_s"] * 1e6)
    end = int(event["end_s"] * 1e6)
    modes = [int(status["nav_state"][i]) for i, stamp in enumerate(status["timestamp"])
             if start <= int(stamp) < end]
    mode_expected = EXPECTED_MODE.get(event["name"])
    corrections = []
    for i, stamp in enumerate(debug["timestamp"]):
        stamp = int(stamp)
        if not start <= stamp < end or name_at(debug, i) != b"AFCR_DA":
            continue
        status_index = bisect.bisect_right(status["timestamp"], stamp) - 1
        if status_index < 0:
            continue
        if mode_expected is not None and int(status["nav_state"][status_index]) != mode_expected:
            continue
        if mode_expected is None and stamp < start + 200_000:
            continue
        corrections.append([float(debug[axis][i]) for axis in "xyz"])
    if any(not math.isfinite(value) for vector in corrections for value in vector):
        return {"classification": "FAIL", "reason": "nonfinite correction"}
    peak = max((abs(value) for vector in corrections for value in vector), default=None)
    mode_observed = mode_expected is None or mode_expected in modes
    if peak is not None and peak > 1e-4:
        classification = "FAIL"
        reason = "candidate correction nonzero in forbidden phase"
    elif len(corrections) < 100 or not mode_observed:
        classification = "PARTIAL"
        reason = "insufficient zero samples or intended mode not observed"
    else:
        classification = "PASS"
        reason = "zero correction with mode and diagnostic samples"
    gps_samples_after = None
    if event["name"] == "gps_loss":
        gps = evaluate.dataset(log, "sensor_gps")
        gps_samples_after = sum(int(stamp) >= start + 500_000 for stamp in gps["timestamp"])
        if gps_samples_after > 0:
            classification = "PARTIAL"
            reason = "GPS stream continued after fault injection"
    return {"classification": classification, "reason": reason,
            "correction_samples": len(corrections), "max_abs_correction_m_s2": peak,
            "nav_states_observed": sorted(set(modes)), "expected_nav_state": mode_expected,
            "gps_samples_after_injection": gps_samples_after,
            "command_rc": event.get("command_rc")}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", type=Path, required=True)
    args = parser.parse_args()
    trials = json.loads((args.run / "results.json").read_text(encoding="utf-8"))
    findings = []
    for trial in trials:
        item = {"boundary": trial["boundary"], "ulog": trial.get("ulog"), "events": []}
        if "ulog" not in trial:
            item.update(classification="PARTIAL", reason=trial.get("error", "no ULog"))
        else:
            for event in trial["events"]:
                if "start_s" in event:
                    item["events"].append({"name": event["name"], **score_boundary(trial["ulog"], event)})
            classifications = {event["classification"] for event in item["events"]}
            item["classification"] = ("FAIL" if "FAIL" in classifications else
                                      "PARTIAL" if "PARTIAL" in classifications else "PASS")
        findings.append(item)
    (args.run / "analysis.json").write_text(json.dumps(findings, indent=2, sort_keys=True) + "\n",
                                               encoding="utf-8")


if __name__ == "__main__":
    main()
