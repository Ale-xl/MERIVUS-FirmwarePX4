/****************************************************************************
 *
 * Copyright (c) 2026 Merivus Industrial. All rights reserved.
 *
 ****************************************************************************/

#include "MotorEffectivenessEstimator.hpp"

#include <gtest/gtest.h>
#include <math.h>

namespace
{

void run_model(MotorEffectivenessEstimator &estimator, float duration, float degraded_motor_scale)
{
	MotorEffectivenessEstimator::Configuration configuration{};
	configuration.lpf_time_constant = 0.03f;
	configuration.excitation_threshold = 0.01f;
	configuration.baseline_time = 2.f;
	configuration.effectiveness_rate_limit = 2.f;
	constexpr float dt = 0.01f;

	for (int sample = 0; sample < static_cast<int>(duration / dt); ++sample) {
		const float t = sample * dt;
		float control[MotorEffectivenessEstimator::MAX_MOTORS] {};
		control[0] = 0.45f + 0.12f * sinf(2.1f * t);
		control[1] = 0.46f + 0.11f * sinf(2.7f * t + 0.4f);
		control[2] = 0.44f + 0.10f * sinf(3.3f * t + 0.8f);
		control[3] = 0.47f + 0.09f * sinf(4.1f * t + 1.2f);
		const float motor_scale[4] {1.f, degraded_motor_scale, 1.f, 1.f};
		float angular_acceleration[3] {};

		for (int motor = 0; motor < 4; ++motor) {
			const float effective_command = control[motor] * motor_scale[motor];
			angular_acceleration[0] += effective_command * (motor == 0 || motor == 3 ? 1.1f : -1.1f);
			angular_acceleration[1] += effective_command * (motor < 2 ? 0.9f : -0.9f);
			angular_acceleration[2] += effective_command * (motor % 2 == 0 ? 0.35f : -0.35f);
		}

		estimator.update(dt, 4, control, angular_acceleration, configuration);
	}
}

TEST(MotorEffectivenessEstimator, RequiresBaselineBeforeValid)
{
	MotorEffectivenessEstimator estimator;
	run_model(estimator, 0.5f, 1.f);
	EXPECT_FALSE(estimator.output().baseline_valid);
	EXPECT_FLOAT_EQ(estimator.output().effectiveness[0], 1.f);
}

TEST(MotorEffectivenessEstimator, TracksContinuousLossAfterHealthyBaseline)
{
	MotorEffectivenessEstimator estimator;
	run_model(estimator, 8.f, 1.f);
	ASSERT_TRUE(estimator.output().baseline_valid);
	run_model(estimator, 12.f, 0.5f);
	EXPECT_LT(estimator.output().effectiveness[1], 0.75f);
	EXPECT_GT(estimator.output().effectiveness[0], 0.85f);
	EXPECT_GT(estimator.output().effectiveness[2], 0.85f);
	EXPECT_GT(estimator.output().effectiveness[3], 0.85f);
	EXPECT_GT(estimator.output().confidence[1], 0.5f);
}

} // namespace
