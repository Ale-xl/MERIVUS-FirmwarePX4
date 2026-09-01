/****************************************************************************
 *
 * Copyright (c) 2026 Merivus Industrial. All rights reserved.
 *
 ****************************************************************************/

#include "MotorEffectivenessEstimator.hpp"

#include <math.h>
#include <string.h>

MotorEffectivenessEstimator::MotorEffectivenessEstimator()
{
	reset();
}

float MotorEffectivenessEstimator::constrain(float value, float minimum, float maximum)
{
	return fminf(fmaxf(value, minimum), maximum);
}

void MotorEffectivenessEstimator::reset()
{
	memset(_coefficient, 0, sizeof(_coefficient));
	memset(_covariance, 0, sizeof(_covariance));
	memset(_nominal_norm, 0, sizeof(_nominal_norm));
	memset(_filtered_control, 0, sizeof(_filtered_control));
	memset(_control_mean, 0, sizeof(_control_mean));
	memset(_control_variance, 0, sizeof(_control_variance));
	memset(_filtered_acceleration, 0, sizeof(_filtered_acceleration));
	memset(&_output, 0, sizeof(_output));

	for (uint8_t i = 0; i < MAX_MOTORS; ++i) {
		_covariance[i][i] = 20.f;
		_effectiveness[i] = 1.f;
		_output.effectiveness[i] = 1.f;
	}

	_baseline_elapsed = 0.f;
	_update_count = 0;
	_motor_count = 0;
	_filter_initialized = false;
	_baseline_valid = false;
}

void MotorEffectivenessEstimator::update(float dt, uint8_t motor_count, const float motor_control[MAX_MOTORS],
		const float angular_acceleration[AXES], const Configuration &configuration)
{
	motor_count = motor_count > MAX_MOTORS ? MAX_MOTORS : motor_count;

	if (motor_count == 0) {
		return;
	}

	if (_motor_count != 0 && motor_count != _motor_count) {
		reset();
	}

	_motor_count = motor_count;
	_output.motor_count = motor_count;
	dt = constrain(dt, 0.001f, 0.1f);
	const float filter_alpha = dt / (fmaxf(configuration.lpf_time_constant, 0.01f) + dt);
	const float statistics_alpha = dt / (2.f + dt);

	if (!_filter_initialized) {
		for (uint8_t i = 0; i < motor_count; ++i) {
			_filtered_control[i] = motor_control[i];
			_control_mean[i] = motor_control[i];
		}

		for (uint8_t axis = 0; axis < AXES; ++axis) {
			_filtered_acceleration[axis] = angular_acceleration[axis];
		}

		_filter_initialized = true;
		return;
	}

	float regressor[MAX_MOTORS] {};
	float excitation_sum = 0.f;

	for (uint8_t i = 0; i < motor_count; ++i) {
		_filtered_control[i] += filter_alpha * (motor_control[i] - _filtered_control[i]);
		const float mean_error = _filtered_control[i] - _control_mean[i];
		_control_mean[i] += statistics_alpha * mean_error;
		_control_variance[i] += statistics_alpha * (mean_error * mean_error - _control_variance[i]);
		_control_variance[i] = fmaxf(_control_variance[i], 0.f);
		regressor[i] = _filtered_control[i];
		excitation_sum += _control_variance[i];
	}

	for (uint8_t axis = 0; axis < AXES; ++axis) {
		_filtered_acceleration[axis] += filter_alpha * (angular_acceleration[axis] - _filtered_acceleration[axis]);
	}

	const float excitation_threshold_sq = configuration.excitation_threshold * configuration.excitation_threshold;
	const bool sufficiently_excited = excitation_sum > excitation_threshold_sq * motor_count;
	float prediction_error[AXES] {};
	float error_norm_sq = 0.f;
	float acceleration_norm_sq = 0.f;

	for (uint8_t axis = 0; axis < AXES; ++axis) {
		float prediction = 0.f;

		for (uint8_t i = 0; i < motor_count; ++i) {
			prediction += _coefficient[axis][i] * regressor[i];
		}

		prediction_error[axis] = _filtered_acceleration[axis] - prediction;
		error_norm_sq += prediction_error[axis] * prediction_error[axis];
		acceleration_norm_sq += _filtered_acceleration[axis] * _filtered_acceleration[axis];
	}

	const float normalized_error = sqrtf(error_norm_sq) / fmaxf(sqrtf(acceleration_norm_sq), 1.f);
	_output.model_residual += statistics_alpha * (normalized_error - _output.model_residual);

	if (sufficiently_excited) {
		float covariance_regressor[MAX_MOTORS] {};
		float denominator = 0.998f;

		for (uint8_t row = 0; row < motor_count; ++row) {
			for (uint8_t column = 0; column < motor_count; ++column) {
				covariance_regressor[row] += _covariance[row][column] * regressor[column];
			}

			denominator += regressor[row] * covariance_regressor[row];
		}

		if (denominator > 1e-6f) {
			float gain[MAX_MOTORS] {};

			for (uint8_t i = 0; i < motor_count; ++i) {
				gain[i] = covariance_regressor[i] / denominator;
			}

			for (uint8_t axis = 0; axis < AXES; ++axis) {
				for (uint8_t i = 0; i < motor_count; ++i) {
					_coefficient[axis][i] = constrain(_coefficient[axis][i] + gain[i] * prediction_error[axis], -200.f, 200.f);
				}
			}

			float updated_covariance[MAX_MOTORS][MAX_MOTORS] {};

			for (uint8_t row = 0; row < motor_count; ++row) {
				for (uint8_t column = 0; column < motor_count; ++column) {
					updated_covariance[row][column] = (_covariance[row][column]
							- gain[row] * covariance_regressor[column]) / 0.998f;
				}
			}

			for (uint8_t row = 0; row < motor_count; ++row) {
				for (uint8_t column = 0; column < motor_count; ++column) {
					_covariance[row][column] = updated_covariance[row][column];
				}
			}
		}

		_baseline_elapsed += dt;
		++_update_count;
	}

	float current_norm[MAX_MOTORS] {};
	bool baseline_coefficients_valid = true;

	for (uint8_t i = 0; i < motor_count; ++i) {
		for (uint8_t axis = 0; axis < AXES; ++axis) {
			current_norm[i] += _coefficient[axis][i] * _coefficient[axis][i];
		}

		current_norm[i] = sqrtf(current_norm[i]);
		const float excitation = sqrtf(_control_variance[i]);
		_output.confidence[i] = constrain(excitation / fmaxf(configuration.excitation_threshold, 0.001f), 0.f, 1.f);

		baseline_coefficients_valid = baseline_coefficients_valid
					      && current_norm[i] > 1e-3f
					      && _output.confidence[i] > 0.2f;
	}

	if (!_baseline_valid && _baseline_elapsed >= configuration.baseline_time && baseline_coefficients_valid) {
		for (uint8_t i = 0; i < motor_count; ++i) {
			_nominal_norm[i] = current_norm[i];
		}

		_baseline_valid = true;
	}

	for (uint8_t i = 0; i < motor_count; ++i) {
		float target_effectiveness = 1.f;

		if (_baseline_valid) {
			target_effectiveness = constrain(current_norm[i] / _nominal_norm[i], 0.f, 1.f);
		}

		const float maximum_change = fmaxf(configuration.effectiveness_rate_limit, 0.01f) * dt;
		const float difference = constrain(target_effectiveness - _effectiveness[i], -maximum_change, maximum_change);
		_effectiveness[i] = constrain(_effectiveness[i] + difference, 0.f, 1.f);
		_output.effectiveness[i] = _effectiveness[i];
		_output.residual[i] = 1.f - _effectiveness[i];
	}

	_output.baseline_valid = _baseline_valid;
}
