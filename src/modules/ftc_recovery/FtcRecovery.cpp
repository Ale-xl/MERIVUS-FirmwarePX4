/****************************************************************************
 *
 * Copyright (c) 2026 Merivus Industrial. All rights reserved.
 *
 ****************************************************************************/

#include "FtcRecovery.hpp"

#include <mathlib/mathlib.h>
#include <px4_platform_common/log.h>

#include <math.h>

namespace
{
constexpr uint32_t TRIGGER_IMPACT = 1u << 0;
constexpr uint32_t TRIGGER_LOSS_OF_CONTROL = 1u << 1;
constexpr uint32_t TRIGGER_MOTOR_FAULT = 1u << 2;
constexpr uint32_t TRIGGER_AUTHORITY = 1u << 3;
constexpr uint32_t INHIBIT_NOT_ARMED = 1u << 0;
constexpr uint32_t INHIBIT_LANDED = 1u << 1;
constexpr uint32_t INHIBIT_LOW_ALTITUDE = 1u << 2;
constexpr uint32_t INHIBIT_STALE_STATE = 1u << 3;
constexpr uint32_t INHIBIT_UNCONTROLLABLE = 1u << 4;
constexpr uint32_t INHIBIT_VEHICLE_TYPE = 1u << 5;
constexpr uint32_t INHIBIT_AUTHORITY = 1u << 6;
}

FtcRecovery::FtcRecovery() :
	ModuleParams(nullptr),
	ScheduledWorkItem(MODULE_NAME, px4::wq_configurations::lp_default)
{
}

FtcRecovery::~FtcRecovery()
{
	ScheduleClear();
}

int FtcRecovery::task_spawn(int argc, char *argv[])
{
	FtcRecovery *instance = new FtcRecovery();

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

bool FtcRecovery::start()
{
	ScheduleOnInterval(20_ms);
	return true;
}

void FtcRecovery::transition(uint8_t state, hrt_abstime now)
{
	if (_state != state) {
		_state = state;
		_state_entered = now;
	}
}

void FtcRecovery::calculateLevelQuaternion(const float q[4], float q_d[4]) const
{
	// Project the current attitude quaternion onto the yaw-only subspace. This remains bounded for large roll/pitch.
	const float yaw_norm = sqrtf(q[0] * q[0] + q[3] * q[3]);
	q_d[0] = yaw_norm > 0.01f ? q[0] / yaw_norm : 1.f;
	q_d[1] = 0.f;
	q_d[2] = 0.f;
	q_d[3] = yaw_norm > 0.01f ? q[3] / yaw_norm : 0.f;
}

void FtcRecovery::generateCandidate(const vehicle_attitude_s &attitude,
		const vehicle_angular_velocity_s &angular_velocity, const vehicle_rates_setpoint_s &current_setpoint,
		ftc_recovery_status_s &status) const
{
	status.candidate_valid = _state >= ftc_recovery_status_s::RATE_DAMPING
				 && _state <= ftc_recovery_status_s::EMERGENCY_LAND;
	status.thrust_body[0] = 0.f;
	status.thrust_body[1] = 0.f;
	status.thrust_body[2] = math::constrain(current_setpoint.thrust_body[2], -1.f, 0.f);
	calculateLevelQuaternion(attitude.q, status.attitude_setpoint_q);
	const float maximum_rate = _param_ftc_rec_rate.get();

	for (uint8_t axis = 0; axis < 3; ++axis) {
		status.body_rate_setpoint[axis] = math::constrain(-_param_ftc_rec_kd.get() * angular_velocity.xyz[axis],
								 -maximum_rate, maximum_rate);
	}

	if (_state >= ftc_recovery_status_s::THRUST_VECTOR_RECOVERY) {
		status.body_rate_setpoint[2] = 0.f;
		status.yaw_sacrifice_requested = true;
	}

	if (_state >= ftc_recovery_status_s::ATTITUDE_RECOVERY) {
		status.body_rate_setpoint[0] = 0.f;
		status.body_rate_setpoint[1] = 0.f;
	}

	if (_state == ftc_recovery_status_s::EMERGENCY_LAND) {
		status.thrust_body[2] = fmaxf(status.thrust_body[2], -0.35f);
		status.yaw_sacrifice_requested = true;
	}
}

void FtcRecovery::Run()
{
	if (should_exit()) {
		ScheduleClear();
		exit_and_cleanup();
		return;
	}

	if (_parameter_update_sub.updated()) {
		parameter_update_s update{};
		_parameter_update_sub.copy(&update);
		updateParams();
	}

	const hrt_abstime now = hrt_absolute_time();
	ftc_extreme_state_s extreme{};
	ftc_control_authority_s authority{};
	motor_health_status_s health{};
	vehicle_attitude_s attitude{};
	vehicle_angular_velocity_s angular_velocity{};
	vehicle_rates_setpoint_s rates_sp{};
	vehicle_local_position_s local_position{};
	vehicle_land_detected_s land{};
	vehicle_status_s vehicle_status{};
	_extreme_sub.copy(&extreme);
	_authority_sub.copy(&authority);
	_health_sub.copy(&health);
	_attitude_sub.copy(&attitude);
	_angular_velocity_sub.copy(&angular_velocity);
	_rates_sp_sub.copy(&rates_sp);
	_local_position_sub.copy(&local_position);
	_land_sub.copy(&land);
	_vehicle_status_sub.copy(&vehicle_status);

	ftc_recovery_status_s status{};
	status.timestamp = now;
	status.state = _state;
	status.intervention_enabled = false; // no arbitration hook exists; FTC_REC_ACT is intentionally inert
	const bool armed = vehicle_status.arming_state == vehicle_status_s::ARMING_STATE_ARMED;
	const bool session_state_fresh = vehicle_status.timestamp != 0 && land.timestamp != 0
					 && now >= vehicle_status.timestamp && now - vehicle_status.timestamp < 1_s
					 && now >= land.timestamp && now - land.timestamp < 1_s;
	const bool state_fresh = attitude.timestamp != 0 && angular_velocity.timestamp != 0
				 && now >= attitude.timestamp && now - attitude.timestamp < 200_ms
				 && now >= angular_velocity.timestamp && now - angular_velocity.timestamp < 200_ms;
	const float altitude = local_position.dist_bottom_valid ? local_position.dist_bottom
			       : (local_position.z_valid ? -local_position.z : 0.f);

	if (!armed) {
		status.inhibit_reason_mask |= INHIBIT_NOT_ARMED;
	}

	if (vehicle_status.vehicle_type != vehicle_status_s::VEHICLE_TYPE_ROTARY_WING) {
		status.inhibit_reason_mask |= INHIBIT_VEHICLE_TYPE;
	}

	if (land.landed) {
		status.inhibit_reason_mask |= INHIBIT_LANDED;
	}

	if (altitude < _param_ftc_rec_alt.get()) {
		status.inhibit_reason_mask |= INHIBIT_LOW_ALTITUDE;
	}

	if (!state_fresh) {
		status.inhibit_reason_mask |= INHIBIT_STALE_STATE;
	}

	if (authority.valid && authority.state == ftc_control_authority_s::UNCONTROLLABLE) {
		status.inhibit_reason_mask |= INHIBIT_UNCONTROLLABLE;
	}

	if (!authority.valid || authority.state == ftc_control_authority_s::ATTITUDE_DEGRADED
	    || authority.state == ftc_control_authority_s::THRUST_INSUFFICIENT) {
		status.inhibit_reason_mask |= INHIBIT_AUTHORITY;
	}

	status.eligible = status.inhibit_reason_mask == 0;

	if (extreme.impact_detected) {
		status.trigger_mask |= TRIGGER_IMPACT;
	}

	if (extreme.loc_state >= ftc_extreme_state_s::LOC_RECOVERY_RECOMMENDED) {
		status.trigger_mask |= TRIGGER_LOSS_OF_CONTROL;
	}

	if (health.failed_mask != 0) {
		status.trigger_mask |= TRIGGER_MOTOR_FAULT;
	}

	if (authority.valid && authority.state >= ftc_control_authority_s::ATTITUDE_DEGRADED) {
		status.trigger_mask |= TRIGGER_AUTHORITY;
	}

	const bool safe_new_session = session_state_fresh && !armed && land.landed && status.trigger_mask == 0;
	const bool terminal_state = _state == ftc_recovery_status_s::ABORTED
				    || _state == ftc_recovery_status_s::FAILED;

	if (!_param_ftc_rec_en.get()) {
		transition(ftc_recovery_status_s::DISABLED, now);

	} else if (_state == ftc_recovery_status_s::DISABLED && (safe_new_session || status.eligible)) {
		transition(ftc_recovery_status_s::MONITORING, now);

	} else if (terminal_state && safe_new_session) {
		transition(ftc_recovery_status_s::MONITORING, now);

	} else if (_state == ftc_recovery_status_s::MONITORING && armed && !land.landed && status.trigger_mask != 0) {
		transition(ftc_recovery_status_s::DISTURBANCE_DETECTED, now);

	} else if (_state == ftc_recovery_status_s::DISTURBANCE_DETECTED) {
		if (authority.state == ftc_control_authority_s::UNCONTROLLABLE) {
			transition(ftc_recovery_status_s::FAILED, now);

		} else if (extreme.loc_state == ftc_extreme_state_s::LOC_UNRECOVERABLE
			   || authority.state == ftc_control_authority_s::THRUST_INSUFFICIENT) {
			transition(ftc_recovery_status_s::EMERGENCY_LAND, now);

		} else if (status.eligible) {
			transition(ftc_recovery_status_s::RATE_DAMPING, now);

		} else if (now - _state_entered > 1_s) {
			transition(ftc_recovery_status_s::ABORTED, now);
		}

	} else if (_state == ftc_recovery_status_s::RATE_DAMPING && extreme.angular_rate < 1.5f) {
		transition(ftc_recovery_status_s::THRUST_VECTOR_RECOVERY, now);

	} else if (_state == ftc_recovery_status_s::THRUST_VECTOR_RECOVERY && now - _state_entered > 500_ms) {
		transition(ftc_recovery_status_s::ATTITUDE_RECOVERY, now);

	} else if (_state == ftc_recovery_status_s::ATTITUDE_RECOVERY && extreme.attitude_error < 0.35f) {
		transition(ftc_recovery_status_s::ALTITUDE_STABILIZATION, now);

	} else if (_state == ftc_recovery_status_s::ALTITUDE_STABILIZATION && now - _state_entered > 1_s) {
		transition(ftc_recovery_status_s::CONTROL_REENTRY, now);

	} else if (_state == ftc_recovery_status_s::CONTROL_REENTRY && extreme.loc_state == ftc_extreme_state_s::LOC_NORMAL) {
		transition(ftc_recovery_status_s::MONITORING, now);
	}

	status.state = _state;
	status.active = _state >= ftc_recovery_status_s::RATE_DAMPING
			&& _state <= ftc_recovery_status_s::EMERGENCY_LAND;
	status.progress = status.active ? math::constrain((now - _state_entered) * 1e-6f, 0.f, 1.f) : 0.f;
	status.state_elapsed = _state_entered == 0 ? 0.f : (now - _state_entered) * 1e-6f;
	generateCandidate(attitude, angular_velocity, rates_sp, status);
	_last_status = status;
	_status_pub.publish(status);
}

int FtcRecovery::print_status()
{
	PX4_INFO("state: %u (%.2f s), eligible: %s, trigger: 0x%08lx, inhibit: 0x%08lx",
		 (unsigned)_last_status.state, (double)_last_status.state_elapsed, _last_status.eligible ? "yes" : "no",
		 (unsigned long)_last_status.trigger_mask,
		 (unsigned long)_last_status.inhibit_reason_mask);
	PX4_INFO("candidate: %s, intervention: disconnected%s", _last_status.candidate_valid ? "valid" : "invalid",
		 _param_ftc_rec_act.get() ? " (requested)" : "");
	return 0;
}

int FtcRecovery::custom_command(int argc, char *argv[])
{
	return print_usage("unknown command");
}

int FtcRecovery::print_usage(const char *reason)
{
	if (reason != nullptr) {
		PX4_WARN("%s", reason);
	}

	PRINT_MODULE_DESCRIPTION("Gated recovery state machine and disconnected recovery setpoint candidate generator.");
	PRINT_MODULE_USAGE_NAME("ftc_recovery", "controller");
	PRINT_MODULE_USAGE_COMMAND("start");
	PRINT_MODULE_USAGE_DEFAULT_COMMANDS();
	return 0;
}

extern "C" __EXPORT int ftc_recovery_main(int argc, char *argv[])
{
	return FtcRecovery::main(argc, argv);
}
