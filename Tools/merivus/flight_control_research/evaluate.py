#!/usr/bin/env python3
"""Score a fixed hover window from SITL ULog truth and allocator telemetry."""

import argparse
import bisect
import json
import math
from pathlib import Path


def dataset(log, name):
    matches = [item for item in log.data_list if item.name == name and item.multi_id == 0]
    if len(matches) != 1:
        raise ValueError(f"required ULog topic missing or ambiguous: {name}")
    return matches[0].data


def sample_at(data, timestamp, maximum_age_us=100_000):
    times = data["timestamp"]
    index = bisect.bisect_right(times, timestamp) - 1
    if index < 0 or timestamp - int(times[index]) > maximum_age_us:
        return None
    return index


def rms(values):
    return math.sqrt(sum(value * value for value in values) / len(values))


def score_arrays(truth, reference, allocator, motors, status, start_us, end_us):
    if end_us <= start_us:
        raise ValueError("window end must follow start")
    xy_errors, z_errors, efforts = [], [], []
    saturation = 0
    samples = 0
    invalid_mode = 0
    for i, timestamp in enumerate(truth["timestamp"]):
        timestamp = int(timestamp)
        if timestamp < start_us or timestamp >= end_us:
            continue
        j = sample_at(reference, timestamp, 150_000)
        k = sample_at(allocator, timestamp, 300_000)
        m = sample_at(motors, timestamp, 150_000)
        s = sample_at(status, timestamp, 1_000_000)
        if j is None or k is None or m is None or s is None:
            continue
        error = [float(truth[axis][i]) - float(reference[axis][j]) for axis in ("x", "y", "z")]
        controls = [float(motors[f"control[{axis}]"][m]) for axis in range(4)]
        if not all(math.isfinite(value) for value in error + controls):
            raise ValueError("nonfinite truth, reference, or motor output")
        xy_errors.append(math.hypot(error[0], error[1]))
        z_errors.append(abs(error[2]))
        efforts.append(sum(value * value for value in controls))
        saturation += not (bool(allocator["thrust_setpoint_achieved"][k])
                           and bool(allocator["torque_setpoint_achieved"][k]))
        invalid_mode += not (int(status["arming_state"][s]) == 2
                             and int(status["nav_state"][s]) in (2, 4))
        samples += 1
    expected = (end_us - start_us) / 20_000
    if samples < 20 or samples < expected * 0.6:
        raise ValueError(f"insufficient aligned truth samples: {samples}")
    metrics = {
        "samples": samples,
        "xy_rmse_m": rms(xy_errors),
        "z_rmse_m": rms(z_errors),
        "xy_peak_m": max(xy_errors),
        "z_peak_m": max(z_errors),
        "motor_effort_mean": sum(efforts) / samples,
        "allocation_failure_fraction": saturation / samples,
        "invalid_mode_fraction": invalid_mode / samples,
    }
    metrics["hard_gate_passed"] = (metrics["xy_peak_m"] < 2.0
                                    and metrics["z_peak_m"] < 1.0
                                    and metrics["allocation_failure_fraction"] < 0.05
                                    and metrics["invalid_mode_fraction"] == 0)
    return metrics


def evaluate(ulog_path, windows_path):
    from pyulog import ULog
    log = ULog(str(ulog_path))
    truth = dataset(log, "vehicle_local_position_groundtruth")
    reference = dataset(log, "vehicle_local_position_setpoint")
    allocator = dataset(log, "control_allocator_status")
    motors = dataset(log, "actuator_motors")
    status = dataset(log, "vehicle_status")
    windows = json.loads(windows_path.read_text(encoding="utf-8"))
    if not isinstance(windows, list) or not windows:
        raise ValueError("windows must be a nonempty list")
    result = {}
    for window in windows:
        name = window["name"]
        if name in result:
            raise ValueError(f"duplicate window: {name}")
        result[name] = score_arrays(truth, reference, allocator, motors, status,
                                    int(window["start_s"] * 1_000_000),
                                    int(window["end_s"] * 1_000_000))
    return result


def compare(baseline, candidate):
    if baseline.keys() != candidate.keys():
        raise ValueError("baseline and candidate windows differ")
    if not all(item["hard_gate_passed"] for item in baseline.values()):
        return {"retain": False, "reason": "baseline hard gate failed"}
    if not all(item["hard_gate_passed"] for item in candidate.values()):
        return {"retain": False, "reason": "candidate hard gate failed"}
    normal = "normal_hover"
    if normal not in baseline:
        raise ValueError("normal_hover window required")
    for key in ("xy_rmse_m", "z_rmse_m", "motor_effort_mean"):
        if candidate[normal][key] > baseline[normal][key] * 1.05:
            return {"retain": False, "reason": f"normal hover regression: {key}"}
    stresses = [name for name in baseline if name != normal]
    if not stresses:
        return {"retain": False, "reason": "no perturbation window"}
    improved = any(candidate[name][key] < baseline[name][key] * 0.9
                   for name in stresses for key in ("xy_rmse_m", "z_rmse_m"))
    return {"retain": improved, "reason": "stress improvement" if improved else "no material stress improvement"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ulog", type=Path, required=True)
    parser.add_argument("--windows", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = evaluate(args.ulog, args.windows)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(args.output)


if __name__ == "__main__":
    main()
