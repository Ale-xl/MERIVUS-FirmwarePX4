# Adaptive Control Allocation

本机 v1.14 的 `control_allocator` 已包含 ActuatorEffectiveness、effectiveness matrix、pseudo-inverse、sequential desaturation 和单电机失效移除逻辑。

当前阶段只实现 shadow contract：读取 nominal `actuator_motors` 与可信 `lambda_i`，计算有界的候选补偿输出并发布专用诊断；不修改 allocator matrix，也不发布候选输出到真实执行器链路。

当前候选算法按 `nominal_i / max(lambda_i, 0.25)` 计算，再对全体候选统一缩放到 `[-1, 1]` 并记录饱和 mask。它用于量化“直接效能补偿”的幅度和饱和风险，不是对 v1.14 sequential desaturation 的第二份复制，也不能宣称等价于重算 effectiveness matrix。真正矩阵 shadow 仍属于 G4。

真正接管前必须完成 G2-G4，并另行设计：矩阵列缩放、置信度/持续时间门控、变化率限制、饱和与可控性判断、nominal fallback。四旋翼完全失去单电机后通常无法同时维持 roll、pitch、yaw、thrust 四自由度，后续策略必须允许降低 yaw 优先级或安全下降，不能承诺正常定点悬停。
