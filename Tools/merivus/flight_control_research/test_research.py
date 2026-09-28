import copy
import json
from pathlib import Path
import unittest

import candidate
import evaluate
import research
import run_sitl


class CandidateTest(unittest.TestCase):
    def setUp(self):
        self.spec = json.loads(Path(__file__).with_name("candidate.json").read_text(encoding="utf-8"))

    def test_checked_in_header_matches_graph(self):
        header = Path(__file__).parents[3] / "src/modules/mc_pos_control/PositionControl/ResearchCandidateSpec.hpp"
        self.assertEqual(header.read_text(encoding="utf-8"), candidate.generate(self.spec))
        self.assertIn("1.0f", candidate.generate(self.spec))

    def test_rejects_unit_mismatch_and_unbounded_output(self):
        wrong = copy.deepcopy(self.spec)
        wrong["nodes"][2]["input"] = "residual"
        wrong["nodes"][2]["op"] = "velocity_gain"
        with self.assertRaisesRegex(ValueError, "expected velocity"):
            candidate.validate(wrong)
        wrong = copy.deepcopy(self.spec)
        wrong["output"] = "compensation"
        with self.assertRaisesRegex(ValueError, "final limit"):
            candidate.validate(wrong)


def telemetry(error=0.1, saturated=False):
    times = [i * 20_000 for i in range(1000)]
    truth = {"timestamp": times, "x": [error] * 1000, "y": [0.0] * 1000,
             "z": [-5.0 + error] * 1000}
    reference = {"timestamp": times[::5], "x": [0.0] * 200, "y": [0.0] * 200,
                 "z": [-5.0] * 200}
    allocator = {"timestamp": times[::10], "thrust_setpoint_achieved": [not saturated] * 100,
                 "torque_setpoint_achieved": [True] * 100}
    motors = {"timestamp": times[::5]}
    for axis in range(4):
        motors[f"control[{axis}]"] = [0.5] * 200
    status = {"timestamp": times[::10], "arming_state": [2] * 100, "nav_state": [4] * 100}
    return truth, reference, allocator, motors, status


class EvaluationTest(unittest.TestCase):
    def test_takeoff_gate_uses_relative_height_and_stationary_vertical_motion(self):
        class Position:
            z = -1.7
            vz = 0.05

        self.assertTrue(run_sitl.hover_reached(Position(), 0.0))
        Position.z = -0.8
        self.assertFalse(run_sitl.hover_reached(Position(), 0.0))
        Position.z = -1.7
        Position.vz = -0.5
        self.assertFalse(run_sitl.hover_reached(Position(), 0.0))

    def test_truth_error_and_allocation_gate(self):
        good = evaluate.score_arrays(*telemetry(), 0, 20_000_000)
        self.assertAlmostEqual(good["xy_rmse_m"], 0.1)
        self.assertAlmostEqual(good["z_rmse_m"], 0.1)
        self.assertTrue(good["hard_gate_passed"])
        bad = evaluate.score_arrays(*telemetry(saturated=True), 0, 20_000_000)
        self.assertFalse(bad["hard_gate_passed"])

    def test_requires_complete_truth(self):
        truth, reference, allocator, motors, status = telemetry()
        truth["timestamp"] = truth["timestamp"][:10]
        with self.assertRaisesRegex(ValueError, "insufficient"):
            evaluate.score_arrays(truth, reference, allocator, motors, status, 0, 20_000_000)

    def test_wrong_flight_mode_fails(self):
        data = telemetry()
        data[-1]["nav_state"] = [0] * 100
        self.assertFalse(evaluate.score_arrays(*data, 0, 20_000_000)["hard_gate_passed"])

    def test_comparison_checks_regression_and_stress(self):
        normal = {"hard_gate_passed": True, "xy_rmse_m": 0.1, "z_rmse_m": 0.1,
                  "motor_effort_mean": 1.0}
        stress = dict(normal, xy_rmse_m=0.4)
        baseline = {"normal_hover": normal, "wind_hover": stress}
        candidate_result = {"normal_hover": dict(normal), "wind_hover": dict(stress, xy_rmse_m=0.3)}
        self.assertTrue(evaluate.compare(baseline, candidate_result)["retain"])
        candidate_result["normal_hover"]["z_rmse_m"] = 0.2
        self.assertFalse(evaluate.compare(baseline, candidate_result)["retain"])


class ResearchTest(unittest.TestCase):
    def test_metadata_does_not_claim_performance(self):
        work = {"DOI": "10.1234/EXAMPLE", "title": ["A control paper"], "publisher": "Example",
                "published": {"date-parts": [[2024, 2]]}}
        record = research.normalize_work(work, "INDI", "2026-09-28T00:00:00Z")
        self.assertEqual(record["review_status"], "METADATA_ONLY")
        self.assertEqual(record["doi"], "10.1234/example")
        self.assertNotIn("performance", record)


if __name__ == "__main__":
    unittest.main()
