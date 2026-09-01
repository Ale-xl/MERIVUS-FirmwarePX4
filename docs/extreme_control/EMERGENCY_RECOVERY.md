# Emergency Recovery

状态：状态机与候选控制律为 `IMPLEMENTED_UNVERIFIED`；控制仲裁和真实介入为 `SKELETON`，且物理断开。

状态：`DISABLED -> MONITORING -> DISTURBANCE_DETECTED -> RATE_DAMPING -> THRUST_VECTOR_RECOVERY -> ATTITUDE_RECOVERY -> ALTITUDE_STABILIZATION -> CONTROL_REENTRY`，并包含 `EMERGENCY_LAND`、`ABORTED`、`FAILED`。

恢复优先级为先抑制危险角速度，再恢复 thrust vector（roll/pitch 优先、yaw 可降级），随后恢复高度并通过 PX4 既有 flight mode、setpoint、Commander 和 failsafe 流程进入安全模式。禁止直接写 motor output 绕过安全机制。

候选控制实现角速度负反馈阻尼、有界 body-rate、保持当前推力幅值的 body-Z/水平姿态四元数恢复、yaw sacrifice 请求和紧急下降候选。它只包含在 `ftc_recovery_status` 中，不发布 `vehicle_rates_setpoint` 或 `vehicle_attitude_setpoint`。

`FTC_REC_EN=0` 且 `FTC_REC_ACT=0`。`FTC_REC_ACT` 在当前版本故意无效；完成独占仲裁、模式所有权、Commander/failsafe 协调和 G7 验证前不得连接。
