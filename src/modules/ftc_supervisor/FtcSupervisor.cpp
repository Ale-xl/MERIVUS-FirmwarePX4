/****************************************************************************
 *
 * Copyright (c) 2026 Merivus Industrial. All rights reserved.
 *
 ****************************************************************************/

#include "FtcSupervisor.hpp"

#include <mathlib/mathlib.h>
#include <px4_platform_common/log.h>

namespace
{
constexpr uint32_t REASON_MODEL_INVALID = 1u << 0;
constexpr uint32_t REASON_MOTOR_DEGRADED = 1u << 1;
constexpr uint32_t REASON_MOTOR_FAILED = 1u << 2;
constexpr uint32_t REASON_AUTHORITY_DEGRADED = 1u << 3;
constexpr uint32_t REASON_EXTREME_STATE = 1u << 4;
constexpr uint32_t REASON_RECOVERY_ABORTED = 1u << 5;
}

FtcSupervisor::FtcSupervisor() :
	ModuleParams(nullptr),
	ScheduledWorkItem(MODULE_NAME, px4::wq_configurations::lp_default)
{
}

FtcSupervisor::~FtcSupervisor()
{
	ScheduleClear();
}

int FtcSupervisor::task_spawn(int argc, char *argv[])
{
	FtcSupervisor *instance = new FtcSupervisor();

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

bool FtcSupervisor::start()
{
	ScheduleOnInterval(100_ms);
	return true;
}

void FtcSupervisor::Run()
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

	motor_health_status_s health{};
	ftc_model_status_s model{};
	ftc_control_authority_s authority{};
	ftc_extreme_state_s extreme{};
	ftc_recovery_status_s recovery{};
	_health_sub.copy(&health);
	_model_sub.copy(&model);
	_authority_sub.copy(&authority);
	_extreme_sub.copy(&extreme);
	_recovery_sub.copy(&recovery);
	ftc_system_status_s status{};
	status.timestamp = hrt_absolute_time();
	status.monitor_enabled = _param_ftc_mon_en.get();
	status.model_valid = model.valid;
	status.control_authority_valid = authority.valid;
	status.recovery_eligible = recovery.eligible;
	status.intervention_enabled = false; // FTC_REC_ACT has no connected arbitration path
	status.degraded_motor_mask = health.degraded_mask;
	status.failed_motor_mask = health.failed_mask;
	status.fault_confirmed = health.failed_mask != 0 || health.degraded_mask != 0;
	status.state = ftc_system_status_s::NORMAL;

	if (!status.monitor_enabled) {
		status.state = ftc_system_status_s::DISABLED;

	} else {
		if (!model.valid) {
			status.reason_mask |= REASON_MODEL_INVALID;
		}

		if (health.degraded_mask != 0) {
			status.reason_mask |= REASON_MOTOR_DEGRADED;
			status.state = ftc_system_status_s::DEGRADED;
		}

		if (health.failed_mask != 0) {
			status.reason_mask |= REASON_MOTOR_FAILED;
			status.state = ftc_system_status_s::FAULT_CONFIRMED;
		}

		if (authority.valid && authority.state != ftc_control_authority_s::FULL_CONTROL) {
			status.reason_mask |= REASON_AUTHORITY_DEGRADED;

			if (status.state < ftc_system_status_s::DEGRADED) {
				status.state = ftc_system_status_s::DEGRADED;
			}
		}

		if (extreme.impact_detected || extreme.loc_state >= ftc_extreme_state_s::LOC_RECOVERY_RECOMMENDED) {
			status.reason_mask |= REASON_EXTREME_STATE;
			status.state = recovery.eligible ? ftc_system_status_s::RECOVERY_READY : ftc_system_status_s::DEGRADED;
		}

		if (recovery.active) {
			status.state = recovery.state == ftc_recovery_status_s::EMERGENCY_LAND
				       ? ftc_system_status_s::EMERGENCY_LAND : ftc_system_status_s::RECOVERY_ACTIVE;
		}

		if (recovery.state == ftc_recovery_status_s::ABORTED || recovery.state == ftc_recovery_status_s::FAILED) {
			status.reason_mask |= REASON_RECOVERY_ABORTED;
			status.state = ftc_system_status_s::FAILED;
		}
	}

	const float model_confidence = model.valid ? model.model_quality : 0.f;
	const float authority_confidence = authority.valid ? authority.minimum_attitude_authority : 0.f;
	const float extreme_confidence = extreme.valid ? 1.f - extreme.loss_of_control_score : 0.f;
	status.system_confidence = math::constrain((model_confidence + authority_confidence + extreme_confidence) / 3.f, 0.f, 1.f);
	_last_status = status;
	_status_pub.publish(status);
}

int FtcSupervisor::print_status()
{
	PX4_INFO("state: %u, confidence %.2f, degraded 0x%04x, failed 0x%04x, intervention: disconnected%s",
		 (unsigned)_last_status.state, (double)_last_status.system_confidence,
		 (unsigned)_last_status.degraded_motor_mask, (unsigned)_last_status.failed_motor_mask,
		 _param_ftc_rec_act.get() ? " (requested)" : "");
	return 0;
}

int FtcSupervisor::custom_command(int argc, char *argv[])
{
	return print_usage("unknown command");
}

int FtcSupervisor::print_usage(const char *reason)
{
	if (reason != nullptr) {
		PX4_WARN("%s", reason);
	}

	PRINT_MODULE_DESCRIPTION("Read-only FTC subsystem state and confidence supervisor.");
	PRINT_MODULE_USAGE_NAME("ftc_supervisor", "controller");
	PRINT_MODULE_USAGE_COMMAND("start");
	PRINT_MODULE_USAGE_DEFAULT_COMMANDS();
	return 0;
}

extern "C" __EXPORT int ftc_supervisor_main(int argc, char *argv[])
{
	return FtcSupervisor::main(argc, argv);
}
