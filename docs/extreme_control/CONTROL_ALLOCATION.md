# Adaptive Control Allocation

本机 v1.14 的 `control_allocator` 已包含 ActuatorEffectiveness、effectiveness matrix、pseudo-inverse、sequential desaturation 和单电机失效移除逻辑。

当前阶段实现真实矩阵 shadow contract。`control_allocator` 仅在 `FTC_CA_SHADOW=1` 时导出它已经生成的 `B_nominal`、trim、linearization point 和 actuator limits；`ftc_control_monitor` 构造

`B_dynamic(:, i) = B_nominal(:, i) * lambda_i`

并调用 PX4 v1.14 原生 `ControlAllocationSequentialDesaturation` 重新分配同一 torque/thrust setpoint。候选电机输出、六轴残差、饱和 mask 和 yaw sacrifice 只写入 `ftc_allocation_shadow`。

旧的逐电机 `nominal/lambda` 补偿已经删除，不再维护第二套近似事实。

`FTC_CA_EN` 默认 0，当前没有执行实现。接管前仍需完成矩阵归一化一致性、置信度/持续时间门控、故障矩阵切换瞬态、优先级仲裁和 fallback 的 SITL/HITL 验证。四旋翼完全失去单电机后通常无法同时维持 roll、pitch、yaw、thrust 四自由度，策略必须允许降低 yaw 优先级或安全下降，不能承诺正常定点悬停。
