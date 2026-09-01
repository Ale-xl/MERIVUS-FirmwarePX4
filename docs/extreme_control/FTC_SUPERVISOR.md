# FTC Supervisor

状态：`IMPLEMENTED_UNVERIFIED`。

`ftc_supervisor` 聚合 motor health、model、control authority、extreme state 和 recovery status，发布唯一顶层 `ftc_system_status`。状态为 DISABLED、NORMAL、DEGRADED、FAULT_CONFIRMED、RECOVERY_READY、RECOVERY_ACTIVE、EMERGENCY_LAND、FAILED，并附 reason mask、关键有效位和 system confidence。

Supervisor 只统一事实和状态，不拥有 actuator、flight mode 或 failsafe 权限。当前 `intervention_enabled` 始终为 false，避免保留参数被误解为已经接管。
