/****************************************************************************
 *
 * Copyright (c) 2026 Merivus Industrial. All rights reserved.
 *
 ****************************************************************************/

#pragma once

#include <stdint.h>

class MotorEffectivenessEstimator
{
public:
	static constexpr uint8_t MAX_MOTORS = 12;
	static constexpr uint8_t AXES = 3;

	struct Configuration {
		float lpf_time_constant{0.2f};
		float excitation_threshold{0.025f};
		float baseline_time{5.f};
		float effectiveness_rate_limit{0.3f};
	};

	struct Output {
		float effectiveness[MAX_MOTORS] {};
		float confidence[MAX_MOTORS] {};
		float residual[MAX_MOTORS] {};
		float model_residual{0.f};
		uint8_t motor_count{0};
		bool baseline_valid{false};
	};

	MotorEffectivenessEstimator();

	void reset();
	void update(float dt, uint8_t motor_count, const float motor_control[MAX_MOTORS],
		    const float angular_acceleration[AXES], const Configuration &configuration);

	const Output &output() const { return _output; }

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
