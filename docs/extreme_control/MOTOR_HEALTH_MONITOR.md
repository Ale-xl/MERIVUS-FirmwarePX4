# Motor Health Monitor

目标模块为 `motor_health_monitor`。它使用 `actuator_motors`、`vehicle_angular_velocity`、`vehicle_acceleration`、`vehicle_status`、`vehicle_land_detected` 和可选 `esc_status`，发布连续健康度、效能、残差、置信度和故障掩码。

第一阶段约束：模块只观察、计算、发布和记录；不写执行器，不改 PID，不切模式。CLI 提供 `start`、`stop`、`status`。参数关闭、未解锁/落地、输入陈旧或激励不足时发布无效状态并重置故障持久计时。

算法初版采用轻量低通、角加速度响应残差、滑动激励统计与持久判定。个体电机辨识受输入相关性和未建模气动力影响，结果必须结合 confidence 与 `model_valid` 使用。

