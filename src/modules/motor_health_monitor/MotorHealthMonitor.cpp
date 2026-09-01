/****************************************************************************
 *
 * Copyright (c) 2026 Merivus Industrial. All rights reserved.
 *
 ****************************************************************************/

#include "MotorHealthMonitor.hpp"

#include <mathlib/mathlib.h>
#include <px4_platform_common/log.h>
#include <px4_platform_common/posix.h>

#include <math.h>
#include <string.h>

MotorHealthMonitor::MotorHealthMonitor() :
	ModuleParams(nullptr),
	ScheduledWorkItem(MODULE_NAME, px4::wq_configurations::lp_default)
{
	resetEstimator();
}

MotorHealthMonitor::~MotorHealthMonitor()
{
	ScheduleClear();
	perf_free(_cycle_perf);
}

int MotorHealthMonitor::task_spawn(int argc, char *argv[])
{
	MotorHealthMonitor *instance = new MotorHealthMonitor();

	if (instance == nullptr) {
		PX4_ERR("alloc failed");
		return PX4_ERROR;
	}

	_object.store(instance);
	_task_id = task_id_is_work_queue;

	if (!instance->start()) {
		delete instance;
		_object.store(nullptr);
		_task_id = -1;
		return PX4_ERROR;
	}

	return PX4_OK;
}

bool MotorHealthMonitor::start()
{
	ScheduleOnInterval(20_ms);
	return true;
}

void MotorHealthMonitor::resetEstimator()
{
	_estimator.reset();
	memset(_degraded_start, 0, sizeof(_degraded_start));
	memset(_failed_start, 0, sizeof(_failed_start));
	memset(&_last_status, 0, sizeof(_last_status));

	for (uint8_t i = 0; i < MotorEffectivenessEstimator::MAX_MOTORS; ++i) {
		_last_status.effectiveness[i] = NAN;
		_last_status.health[i] = NAN;
		_last_status.confidence[i] = 0.f;
		_last_status.residual[i] = NAN;
	}
}

uint8_t MotorHealthMonitor::motorCount(const actuator_motors_s &actuator_motors) const
{
	uint8_t count = 0;

	for (uint8_t i = 0; i < actuator_motors_s::NUM_CONTROLS; ++i) {
		if (PX4_ISFINITE(actuator_motors.control[i])) {
			count = i + 1;
		}
	}

	return count;
}

void MotorHealthMonitor::publishDisabled(hrt_abstime now)
{
	if (now - _last_disabled_publish < 1_s) {
		return;
	}

	_last_status.timestamp = now;
	_last_status.state = motor_health_status_s::STATE_DISABLED;
	_last_status.model_valid = false;
	_last_status.degraded_mask = 0;
	_last_status.failed_mask = 0;
	_motor_health_status_pub.publish(_last_status);
	_last_disabled_publish = now;
}

void MotorHealthMonitor::publishShadow(const actuator_motors_s &actuator_motors,
		const motor_health_status_s &health_status)
{
	ftc_allocation_shadow_s shadow{};
	shadow.timestamp = hrt_absolute_time();
	shadow.timestamp_sample = actuator_motors.timestamp_sample;
	shadow.motor_count = health_status.motor_count;
	shadow.valid = health_status.model_valid;
	float maximum_magnitude = 1.f;

	for (uint8_t i = 0; i < ftc_allocation_shadow_s::NUM_MOTORS; ++i) {
		shadow.nominal[i] = NAN;
		shadow.candidate[i] = NAN;
		shadow.effectiveness[i] = NAN;
	}

	for (uint8_t i = 0; i < shadow.motor_count; ++i) {
		shadow.nominal[i] = actuator_motors.control[i];
		shadow.effectiveness[i] = health_status.effectiveness[i];
		const float bounded_effectiveness = fmaxf(health_status.effectiveness[i], 0.25f);
		shadow.candidate[i] = actuator_motors.control[i] / bounded_effectiveness;
		maximum_magnitude = fmaxf(maximum_magnitude, fabsf(shadow.candidate[i]));

		if (fabsf(shadow.candidate[i]) > 1.f) {
			shadow.saturated_mask |= 1u << i;
		}
	}

	for (uint8_t i = 0; i < shadow.motor_count; ++i) {
		shadow.candidate[i] = math::constrain(shadow.candidate[i] / maximum_magnitude, -1.f, 1.f);
	}

	_allocation_shadow_pub.publish(shadow);
}

void MotorHealthMonitor::Run()
{
	if (should_exit()) {
		ScheduleClear();
		exit_and_cleanup();
		return;
	}

	perf_begin(_cycle_perf);
	const hrt_abstime now = hrt_absolute_time();

	if (_parameter_update_sub.updated()) {
		parameter_update_s parameter_update{};
		_parameter_update_sub.copy(&parameter_update);
		updateParams();
	}

	_vehicle_status_sub.update(&_vehicle_status);
	_land_detected_sub.update(&_land_detected);
	_esc_status_sub.update(&_esc_status);
	const bool armed = _vehicle_status.arming_state == vehicle_status_s::ARMING_STATE_ARMED;

	if (!armed && _was_armed) {
		resetEstimator();
	}

	_was_armed = armed;

	if (!_param_ftc_mon_en.get()) {
		if (_was_enabled) {
			resetEstimator();
		}

		_was_enabled = false;
		publishDisabled(now);
		perf_end(_cycle_perf);
		return;
	}

	_was_enabled = true;

	actuator_motors_s actuator_motors{};
	vehicle_angular_velocity_s angular_velocity{};
	const bool motors_available = _actuator_motors_sub.copy(&actuator_motors);
	const bool angular_velocity_available = _angular_velocity_sub.copy(&angular_velocity);
	const uint8_t motor_count = motors_available ? motorCount(actuator_motors) : 0;
	float mean_motor_command = 0.f;

	for (uint8_t i = 0; i < motor_count; ++i) {
		mean_motor_command += fmaxf(actuator_motors.control[i], 0.f);
	}

	if (motor_count > 0) {
		mean_motor_command /= motor_count;
	}

	const bool input_fresh = motors_available && angular_velocity_available
				 && now >= actuator_motors.timestamp && now - actuator_motors.timestamp < 200_ms
				 && now >= angular_velocity.timestamp && now - angular_velocity.timestamp < 200_ms;
	const bool flight_state_fresh = _vehicle_status.timestamp != 0
					&& now >= _vehicle_status.timestamp && now - _vehicle_status.timestamp < 1_s
					&& _land_detected.timestamp != 0
					&& now >= _land_detected.timestamp && now - _land_detected.timestamp < 1_s;
	const bool flight_valid = armed && flight_state_fresh && !_land_detected.landed && input_fresh && motor_count >= 2
				  && mean_motor_command >= _param_ftc_min_thr.get();
	const float dt = _last_run == 0 ? 0.02f : math::constrain((now - _last_run) * 1e-6f, 0.001f, 0.1f);
	_last_run = now;

	if (flight_valid) {
		MotorEffectivenessEstimator::Configuration configuration{};
		configuration.lpf_time_constant = _param_ftc_lpf_tc.get();
		configuration.excitation_threshold = _param_ftc_exc_min.get();
		configuration.baseline_time = _param_ftc_base_t.get();
		configuration.effectiveness_rate_limit = _param_ftc_est_rate.get();
		_estimator.update(dt, motor_count, actuator_motors.control, angular_velocity.xyz_derivative, configuration);
	}

	const MotorEffectivenessEstimator::Output &estimate = _estimator.output();
	motor_health_status_s status{};
	status.timestamp = now;
	status.timestamp_sample = angular_velocity.timestamp_sample;
	status.motor_count = motor_count;
	status.esc_data_available = _esc_status.timestamp != 0
				    && now >= _esc_status.timestamp
				    && now - _esc_status.timestamp < 1_s;
	status.model_residual = estimate.model_residual;
	uint8_t confident_motors = 0;

	for (uint8_t i = 0; i < motor_health_status_s::NUM_MOTORS; ++i) {
		status.effectiveness[i] = NAN;
		status.health[i] = NAN;
		status.confidence[i] = 0.f;
		status.residual[i] = NAN;
	}

	for (uint8_t i = 0; i < motor_count; ++i) {
		status.effectiveness[i] = estimate.effectiveness[i];
		status.health[i] = estimate.effectiveness[i];
		status.confidence[i] = flight_valid ? estimate.confidence[i] : 0.f;
		status.residual[i] = estimate.residual[i];

		if (status.confidence[i] >= _param_ftc_conf_min.get()) {
			++confident_motors;
		}
	}

	status.model_valid = flight_valid && estimate.baseline_valid
			     && confident_motors == motor_count
			     && estimate.model_residual <= _param_ftc_res_thr.get();
	status.state = status.model_valid ? motor_health_status_s::STATE_VALID
		       : (flight_valid && !estimate.baseline_valid ? motor_health_status_s::STATE_CALIBRATING
			  : motor_health_status_s::STATE_INVALID);
	const hrt_abstime persistence = static_cast<hrt_abstime>(_param_ftc_fail_t.get() * 1_s);

	for (uint8_t i = 0; i < motor_count; ++i) {
		if (status.model_valid && status.health[i] < _param_ftc_hlth_min.get()) {
			if (_degraded_start[i] == 0) {
				_degraded_start[i] = now;
			}

			if (now - _degraded_start[i] >= persistence) {
				status.degraded_mask |= 1u << i;
			}

		} else {
			_degraded_start[i] = 0;
		}

		if (status.model_valid && status.health[i] < _param_ftc_fail_min.get()) {
			if (_failed_start[i] == 0) {
				_failed_start[i] = now;
			}

			if (now - _failed_start[i] >= persistence) {
				status.failed_mask |= 1u << i;
			}

		} else {
			_failed_start[i] = 0;
		}
	}

	_last_status = status;
	_motor_health_status_pub.publish(status);

	if (_param_ftc_ca_shadow.get()) {
		publishShadow(actuator_motors, status);
	}

	perf_end(_cycle_perf);
}

int MotorHealthMonitor::print_status()
{
	PX4_INFO("Motor health monitor: %s", _param_ftc_mon_en.get() ? "ACTIVE" : "DISABLED");
	PX4_INFO("state: %u, model valid: %s, motors: %u, degraded: 0x%04x, failed: 0x%04x",
		 (unsigned)_last_status.state, _last_status.model_valid ? "yes" : "no", (unsigned)_last_status.motor_count,
		 (unsigned)_last_status.degraded_mask, (unsigned)_last_status.failed_mask);

	for (uint8_t i = 0; i < _last_status.motor_count; ++i) {
		PX4_INFO("motor %u: effectiveness %.3f, confidence %.3f, residual %.3f", (unsigned)i,
			 (double)_last_status.effectiveness[i], (double)_last_status.confidence[i],
			 (double)_last_status.residual[i]);
	}

	perf_print_counter(_cycle_perf);
	return 0;
}

int MotorHealthMonitor::custom_command(int argc, char *argv[])
{
	return print_usage("unknown command");
}

int MotorHealthMonitor::print_usage(const char *reason)
{
	if (reason != nullptr) {
		PX4_WARN("%s", reason);
	}

	PRINT_MODULE_DESCRIPTION(
		R"DESCR_STR(
### Description
Read-only motor effectiveness and health monitor for PX4 v1.14. The module estimates continuous motor health from
actuator commands and measured angular acceleration, publishes ULog-visible diagnostics, and can calculate an
adaptive allocation candidate in shadow mode. It never publishes actuator commands or controller setpoints.
)DESCR_STR");
	PRINT_MODULE_USAGE_NAME("motor_health_monitor", "controller");
	PRINT_MODULE_USAGE_COMMAND("start");
	PRINT_MODULE_USAGE_DEFAULT_COMMANDS();
	return 0;
}

extern "C" __EXPORT int motor_health_monitor_main(int argc, char *argv[])
{
	return MotorHealthMonitor::main(argc, argv);
}
