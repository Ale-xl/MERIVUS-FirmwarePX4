# ULog 分析合同

默认日志应包含：

- FTC 电机健康 topic：effectiveness、health、confidence、residual、degraded/failed mask、model_valid。
- FTC allocation shadow topic：nominal、candidate、应用的 lambda、饱和 mask、shadow_valid。
- SITL 注入 topic：`ftc_simulation_status` 的目标/实际效能、电机索引和间歇状态。
- 关联 topic：`actuator_motors`、`vehicle_torque_setpoint`、`vehicle_thrust_setpoint`、`vehicle_angular_velocity`、`vehicle_acceleration`、`control_allocator_status`、`vehicle_status`、`vehicle_land_detected`、可选 `esc_status`。

分析窗口至少覆盖异常前、异常发生和异常后的估计收敛。首先用正常 hover、位置、稳定、roll/pitch/yaw、爬升和下降日志建立 residual/confidence 基线，再确定实机阈值。
