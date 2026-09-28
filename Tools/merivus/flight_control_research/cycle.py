#!/usr/bin/env python3
"""Run a reproducible candidate-versus-PX4 SITL research cycle on Linux."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

import candidate
import evaluate


ROOT = Path(__file__).resolve().parents[3]
SPEC = Path(__file__).with_name("candidate.json")
HEADER = ROOT / "src/modules/mc_pos_control/PositionControl/ResearchCandidateSpec.hpp"
ALLOWED_EDITS = {
    "Tools/merivus/flight_control_research/candidate.json",
    "src/modules/mc_pos_control/PositionControl/ResearchCandidateSpec.hpp",
}


def run(command, *, cwd=ROOT):
    subprocess.run(command, cwd=cwd, check=True)


def changed_files():
    changed = subprocess.check_output(["git", "diff", "--name-only", "HEAD"], cwd=ROOT, text=True).splitlines()
    untracked = subprocess.check_output(["git", "ls-files", "--others", "--exclude-standard"],
                                        cwd=ROOT, text=True).splitlines()
    return set(changed + untracked)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--dialect", type=Path, required=True)
    parser.add_argument("--seeds", type=int, nargs="+", default=[1, 2])
    args = parser.parse_args()
    if os.name != "posix":
        raise SystemExit("PX4 Gazebo Classic cycle requires a Linux host")
    if len(set(args.seeds)) != len(args.seeds) or any(seed < 0 for seed in args.seeds):
        raise ValueError("seeds must be distinct nonnegative integers")
    unexpected = changed_files() - ALLOWED_EDITS
    if unexpected:
        raise RuntimeError("research infrastructure changed: " + ", ".join(sorted(unexpected)))
    spec = json.loads(SPEC.read_text(encoding="utf-8"))
    generated = candidate.generate(spec)
    if HEADER.read_text(encoding="utf-8") != generated:
        raise RuntimeError("generated candidate header differs from candidate.json")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    scripts = ("candidate.py", "evaluate.py", "run_sitl.py", "cycle.py", "research.py", "test_research.py")
    manifest = {
        "git_head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "candidate_sha256": hashlib.sha256(SPEC.read_bytes()).hexdigest(),
        "scripts_sha256": {name: hashlib.sha256(SPEC.with_name(name).read_bytes()).hexdigest()
                           for name in scripts},
        "seeds": args.seeds,
    }
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    run([sys.executable, "-m", "unittest", "discover", "-s", str(SPEC.parent), "-p", "test_research.py"])
    run(["make", "px4_sitl_default", "sitl_gazebo-classic"])
    paired = []
    for seed in args.seeds:
        baseline, active = {}, {}
        for scenario in ("normal", "wind", "payload"):
            for mode, collection in (("off", baseline), ("active", active)):
                trial_output = output / f"seed{seed}_{scenario}_{mode}"
                run([sys.executable, str(SPEC.with_name("run_sitl.py")), "--repo", str(ROOT),
                     "--output", str(trial_output), "--dialect", str(args.dialect.resolve()),
                     "--mode", mode, "--scenario", scenario, "--seed", str(seed)])
                ulog = Path((trial_output / "ulog_path.txt").read_text(encoding="utf-8").strip())
                collection.update(evaluate.evaluate(ulog, trial_output / "windows.json"))
        decision = evaluate.compare(baseline, active)
        paired.append({"seed": seed, "baseline": baseline, "active": active, "decision": decision})
        (output / "results.json").write_text(json.dumps(paired, indent=2, sort_keys=True) + "\n",
                                                encoding="utf-8")
    retained = all(item["decision"]["retain"] for item in paired)
    (output / "decision.json").write_text(json.dumps({"retain_for_further_sitl": retained,
                                                      "seed_count": len(paired)}, indent=2) + "\n",
                                              encoding="utf-8")
    print("retain for further SITL" if retained else "reject candidate")


if __name__ == "__main__":
    main()
