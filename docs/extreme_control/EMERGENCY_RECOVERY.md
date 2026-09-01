# Emergency Recovery

状态：监督状态机契约已定义，恢复控制未启用。

预期状态：`NORMAL -> DISTURBANCE_DETECTED -> RATE_DAMPING -> ATTITUDE_RECOVERY -> STABILIZE -> LAND/POSITION/MANUAL`。

恢复优先级为先抑制危险角速度，再恢复 thrust vector（roll/pitch 优先、yaw 可降级），随后恢复高度并通过 PX4 既有 flight mode、setpoint、Commander 和 failsafe 流程进入安全模式。禁止直接写 motor output 绕过安全机制。

`FTC_REC_EN` 必须默认 0；在完成 G7 前不建立实机控制接管路径。
