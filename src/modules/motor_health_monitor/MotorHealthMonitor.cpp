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
	memset(_fault_probability_lpf, 0, sizeof(_fault_probability_lpf));
	memset(_effectiveness_change_lpf, 0, sizeof(_effectiveness_change_lpf));
	memset(&_last_status, 0, sizeof(_last_status));

	for (uint8_t i = 0; i < MotorEffectivenessEstimator::MAX_MOTORS; ++i) {
		_last_status.effectiveness[i] = NAN;
		_last_status.health[i] = NAN;
		_last_status.confidence[i] = 0.f;
		_last_status.residual[i] = NAN;
		_previous_effectiveness[i] = 1.f;
	}

	_filtered_acceleration_magnitude = 0.f;
	_acceleration_filter_initialized = false;
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

void MotorHealthMonitor::classifyFaults(hrt_abstime now, const actuator_motors_s &actuator_motors,
		motor_health_status_s &status, float dt)
{
	const float probability_alpha = math::constrain(dt / (0.4f + dt), 0.f, 1.f);
	const bool esc_available = status.esc_data_available;
	uint8_t localized_faults = 0;

	for (uint8_t i = 0; i < status.motor_count; ++i) {
		const float degradation = status.model_valid ? 1.f - status.effectiveness[i] : 0.f;
		const float change_rate = fabsf(status.effectiveness[i] - _previous_effectiveness[i]) / fmaxf(dt, 0.001f);
		_effectiveness_change_lpf[i] += probability_alpha * (change_rate - _effectiveness_change_lpf[i]);
		_previous_effectiveness[i] = status.effectiveness[i];

		bool esc_online = true;
		bool esc_failed = false;
		bool motor_stuck = false;
		bool rpm_stopped = false;

		if (esc_available && i < _esc_status.esc_count) {
			esc_online = (_esc_status.esc_online_flags & (1u << i)) != 0;
			const uint16_t failures = _esc_status.esc[i].failures;
			motor_stuck = (failures & (1u << esc_report_s::FAILURE_MOTOR_STUCK)) != 0;
			esc_failed = failures != 0 && !motor_stuck;
			rpm_stopped = actuator_motors.control[i] > 0.25f && _esc_status.esc[i].esc_rpm >= 0
				      && _esc_status.esc[i].esc_rpm < 100;
		}

		float evidence = degradation * status.confidence[i];
		evidence = fmaxf(evidence, (!esc_online || esc_failed || motor_stuck || rpm_stopped) ? 1.f : 0.f);
		_fault_probability_lpf[i] += probability_alpha * (evidence - _fault_probability_lpf[i]);
		status.fault_probability[i] = math::constrain(_fault_probability_lpf[i], 0.f, 1.f);
		status.fault_confidence[i] = esc_available
					     ? fmaxf(status.confidence[i], (!esc_online || esc_failed || motor_stuck) ? 1.f : 0.f)
					     : status.confidence[i] * 0.8f;
		status.fault_persistence[i] = _degraded_start[i] == 0 ? 0.f : (now - _degraded_start[i]) * 1e-6f;
		status.fault_type[i] = motor_health_status_s::FAULT_NONE;

		if (status.fault_probability[i] < _param_ftc_fault_p.get()) {
			if (status.vibration_score > 1.f && degradation < 0.1f) {
				status.fault_type[i] = motor_health_status_s::FAULT_MECHANICAL_IMBALANCE;
			}

			continue;
		}

		++localized_faults;

		if (!esc_online || esc_failed) {
			status.fault_type[i] = motor_health_status_s::FAULT_ESC_OR_POWER_FAILURE;

		} else if (motor_stuck || rpm_stopped) {
			status.fault_type[i] = motor_health_status_s::FAULT_MOTOR_STOP;

		} else if (_effectiveness_change_lpf[i] > 0.25f && status.fault_persistence[i] > 1.f) {
			status.fault_type[i] = motor_health_status_s::FAULT_INTERMITTENT_PROPULSION_FAILURE;

		} else if (status.vibration_score > 1.f && degradation > 0.15f && status.fault_confidence[i] > 0.6f) {
			status.fault_type[i] = motor_health_status_s::FAULT_PROP_DAMAGE;

		} else if (status.model_valid && status.fault_confidence[i] > 0.75f) {
			status.fault_type[i] = motor_health_status_s::FAULT_MOTOR_DEGRADATION;

		} else {
			status.fault_type[i] = motor_health_status_s::FAULT_UNKNOWN_PROPULSION_DEGRADATION;
		}
	}

	if (localized_faults == 0 && status.external_disturbance_score >= _param_ftc_fault_ext.get()) {
		for (uint8_t i = 0; i < status.motor_count; ++i) {
			status.fault_type[i] = motor_health_status_s::FAULT_EXTERNAL_DISTURBANCE;
		}

	} else if (localized_faults == 0 && status.model_residual > _param_ftc_res_thr.get() && status.excitation > 0.5f) {
		for (uint8_t i = 0; i < status.motor_count; ++i) {
			status.fault_type[i] = motor_health_status_s::FAULT_MODEL_MISMATCH;
		}
	}
}

void MotorHealthMonitor::publishModelStatus(hrt_abstime now,
		const vehicle_angular_velocity_s &angular_velocity, const motor_health_status_s &health_status)
{
	ftc_model_status_s model{};
	model.timestamp = now;
	model.timestamp_sample = angular_velocity.timestamp_sample;
	model.estimator_type = 1; // bounded RLS
	model.motor_count = health_status.motor_count;
	model.effectiveness_valid = health_status.model_valid;
	model.valid = health_status.model_valid;
	model.excitation = health_status.excitation;
	model.model_quality = health_status.model_valid
			      ? math::constrain((1.f - math::constrain(health_status.model_residual, 0.f, 1.f))
						* health_status.excitation, 0.f, 1.f) : 0.f;
	model.mass = NAN;
	model.mass_valid = false;
	model.inertia[0] = _param_ftc_est_ixx.get();
	model.inertia[1] = _param_ftc_est_iyy.get();
	model.inertia[2] = _param_ftc_est_izz.get();
	model.inertia_valid = false;
	model.cg_offset[0] = model.cg_offset[1] = model.cg_offset[2] = NAN;
	model.cg_valid = false;

	const float i_omega[3] {model.inertia[0] * angular_velocity.xyz[0],
				model.inertia[1] * angular_velocity.xyz[1], model.inertia[2] * angular_velocity.xyz[2]};
	const float gyroscopic[3] {
		angular_velocity.xyz[1] * i_omega[2] - angular_velocity.xyz[2] * i_omega[1],
		angular_velocity.xyz[2] * i_omega[0] - angular_velocity.xyz[0] * i_omega[2],
		angular_velocity.xyz[0] * i_omega[1] - angular_velocity.xyz[1] * i_omega[0]
	};
	float rotational_norm_sq = 0.f;

	for (uint8_t axis = 0; axis < 3; ++axis) {
		const float torque = model.inertia[axis] * angular_velocity.xyz_derivative[axis] + gyroscopic[axis];
		rotational_norm_sq += torque * torque;
	}

	model.rotational_residual = sqrtf(rotational_norm_sq);

	for (uint8_t i = 0; i < ftc_model_status_s::NUM_MOTORS; ++i) {
		model.motor_effectiveness[i] = health_status.effectiveness[i];
		model.motor_confidence[i] = health_status.confidence[i];
	}

	_model_status_pub.publish(model);
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
	vehicle_acceleration_s acceleration{};
	const bool motors_available = _actuator_motors_sub.copy(&actuator_motors);
	const bool angular_velocity_available = _angular_velocity_sub.copy(&angular_velocity);
	const bool acceleration_available = _acceleration_sub.copy(&acceleration);
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
		configuration.forgetting_factor = _param_ftc_est_forg.get();
		configuration.minimum_effectiveness = _param_ftc_est_lmin.get();
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
	status.excitation = estimate.excitation;
	status.imu_only = !status.esc_data_available;
	status.maneuver_intensity = math::constrain(sqrtf(angular_velocity.xyz[0] * angular_velocity.xyz[0]
				    + angular_velocity.xyz[1] * angular_velocity.xyz[1]
				    + angular_velocity.xyz[2] * angular_velocity.xyz[2]) / 5.f, 0.f, 1.f);

	if (acceleration_available) {
		const float acceleration_magnitude = sqrtf(acceleration.xyz[0] * acceleration.xyz[0]
						   + acceleration.xyz[1] * acceleration.xyz[1]
						   + acceleration.xyz[2] * acceleration.xyz[2]);

		if (!_acceleration_filter_initialized) {
			_filtered_acceleration_magnitude = acceleration_magnitude;
			_acceleration_filter_initialized = true;
		}

		const float vibration = fabsf(acceleration_magnitude - _filtered_acceleration_magnitude);
		_filtered_acceleration_magnitude += math::constrain(dt / (0.5f + dt), 0.f, 1.f)
						* (acceleration_magnitude - _filtered_acceleration_magnitude);
		status.vibration_score = vibration / fmaxf(_param_ftc_fault_vib.get(), 0.1f);
		status.external_disturbance_score = math::constrain((fabsf(acceleration_magnitude - 9.81f) / 20.f
								 + estimate.model_residual) * 0.5f
								 * (1.f - 0.5f * status.maneuver_intensity), 0.f, 1.f);
	}
	uint8_t confident_motors = 0;

	for (uint8_t i = 0; i < motor_health_status_s::NUM_MOTORS; ++i) {
		status.effectiveness[i] = NAN;
		status.health[i] = NAN;
		status.confidence[i] = 0.f;
		status.residual[i] = NAN;
		status.fault_probability[i] = 0.f;
		status.fault_confidence[i] = 0.f;
		status.fault_persistence[i] = 0.f;
		status.fault_type[i] = motor_health_status_s::FAULT_NONE;
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

	classifyFaults(now, actuator_motors, status, dt);

	_last_status = status;
	_motor_health_status_pub.publish(status);
	publishModelStatus(now, angular_velocity, status);

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
