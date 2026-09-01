/****************************************************************************
 *
 * Copyright (c) 2026 Merivus Industrial. All rights reserved.
 *
 ****************************************************************************/

#pragma once

#include <stdint.h>

class EffectivenessEstimator
{
public:
	static constexpr uint8_t MAX_MOTORS = 12;
	static constexpr uint8_t AXES = 3;

	struct Configuration {
		float lpf_time_constant{0.2f};
		float excitation_threshold{0.025f};
		float baseline_time{5.f};
		float effectiveness_rate_limit{0.3f};
		float forgetting_factor{0.995f};
		float minimum_effectiveness{0.1f};
	};

	struct Output {
		float effectiveness[MAX_MOTORS] {};
		float confidence[MAX_MOTORS] {};
		float residual[MAX_MOTORS] {};
		float model_residual{0.f};
		float excitation{0.f};
		uint8_t motor_count{0};
		bool baseline_valid{false};
	};

	virtual ~EffectivenessEstimator() = default;
	virtual void reset() = 0;
	virtual void update(float dt, uint8_t motor_count, const float motor_control[MAX_MOTORS],
			    const float angular_acceleration[AXES], const Configuration &configuration) = 0;
	virtual const Output &output() const = 0;
};
