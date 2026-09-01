# 控制权限

状态：`IMPLEMENTED_UNVERIFIED`。

`ftc_control_monitor` 用 `B_dynamic`、actuator limits、当前电机输出和 headroom 计算 roll、pitch、yaw、thrust 的归一化剩余权限。状态集合为 FULL_CONTROL、DEGRADED_CONTROL、YAW_UNCONTROLLABLE、ATTITUDE_DEGRADED、THRUST_INSUFFICIENT、RECOVERY_ONLY、UNCONTROLLABLE。

当前权限比值使用动态/标称矩阵列绝对能力之比，并单独发布 actuator headroom 与 saturation mask。它适合 shadow 趋势和恢复资格判断，但尚未完成可达控制集合、符号方向不对称、倾转/多矩阵机型和实时优先级优化验证。

Yaw 权限低而 roll/pitch 尚可时发布 yaw sacrifice 请求；这只是接口，不修改真实 allocator 优先级。
