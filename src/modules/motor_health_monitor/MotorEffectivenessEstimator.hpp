/****************************************************************************
 *
 * Copyright (c) 2026 Merivus Industrial. All rights reserved.
 *
 ****************************************************************************/

#pragma once

#include "EffectivenessEstimator.hpp"

class MotorEffectivenessEstimator : public EffectivenessEstimator
{
public:
	MotorEffectivenessEstimator();

	void reset() override;
	void update(float dt, uint8_t motor_count, const float motor_control[MAX_MOTORS],
		    const float angular_acceleration[AXES], const Configuration &configuration) override;

	const Output &output() const override { return _output; }

private:
	static float constrain(float value, float minimum, float maximum);

	float _coefficient[AXES][MAX_MOTORS] {};
	float _covariance[MAX_MOTORS][MAX_MOTORS] {};
	float _nominal_norm[MAX_MOTORS] {};
	float _filtered_control[MAX_MOTORS] {};
	float _control_mean[MAX_MOTORS] {};
	float _control_variance[MAX_MOTORS] {};
	float _filtered_acceleration[AXES] {};
	float _effectiveness[MAX_MOTORS] {};
	float _baseline_elapsed{0.f};
	uint32_t _update_count{0};
	uint8_t _motor_count{0};
	bool _filter_initialized{false};
	bool _baseline_valid{false};
	Output _output{};
};
