# FTC 电机效能失败分析

日期：2026-09-07  
基线：`research/extreme-control-v1` / `e2d59728b93b5bf3aa300ac175373fa75c255f58`

## 结论

`EFF_70_FINAL_BLOCKED.ulg` 证明，M1 的 0.70 效能在 `130.076000 s` 瞬时生效后，控制器只有约 `104 ms` 就把该电机命令推到上限，`120 ms` 后分配器持续报告饱和且力矩/推力设定值不可同时实现。姿态误差在 `636 ms` 后超过 `0.6 rad`，角速度在 `1.356 s` 后超过 `5 rad/s`，NED 本地位置的 `z` 在 `1.836 s` 后越过地面平面。

这不是一个“先得到稳定 0.70 估计，再缓慢失效”的样本；飞行器先进入控制饱和，留给在线辨识的正常闭环观测窗口不足。不得通过降低 `FTC_CONF_MIN`、降低激励门、放宽 `FTC_RES_THR` 或强行保持 `model_valid` 把该样本改判为通过。

更重要的是，四份 ULog 均缺失 `motor_health_status`、`ftc_model_status`、`ftc_control_authority`、`ftc_extreme_state`、`ftc_recovery_status` 和 `ftc_system_status`。因此现有二进制证据无法给出“置信度首次跌破 0.60”“残差首次超过 1.0”“模型首次 invalid”的精确时间。此前人工 listener 观察只能证明这些状态出现过，不能替代时间对齐的 ULog 证据。

## 证据范围

只读解析了：

- `NORMAL.ulg`
- `EFF_100.ulg`
- `EFF_70_FINAL_BLOCKED.ulg`
- `EFF_40_FINAL_BLOCKED.ulg`

证据目录为 `/home/cwkj/MERIVUS/test-artifacts/20260903-013501/`。解析使用临时安装在 `/tmp/merivus-pyulog` 的 `pyulog 1.2.4`，未修改系统 Python，也未运行新的故障飞行。

## 0.70 精确时间线

下表以 `ftc_simulation_status` 首次满足 `enabled=true && target_effectiveness<0.999` 为 `T0`。时间均来自同一 ULog 的 PX4 单调时钟。

| 事件 | ULog 时间 | 相对 T0 | 证据 |
| --- | ---: | ---: | --- |
| M1 目标/实际效能变为 0.70 | 130.076000 s | 0 ms | `ftc_simulation_status`，`motor_index=0` |
| M1 命令连续超过 0.98 | 130.180000 s | 104 ms | `actuator_motors.control[0]` |
| 分配器持续饱和 | 130.196000 s | 120 ms | `control_allocator_status.actuator_saturation[]` |
| 力矩/推力设定值不能同时实现 | 130.196000 s | 120 ms | `torque_setpoint_achieved` / `thrust_setpoint_achieved` |
| 姿态误差连续超过 0.6 rad | 130.712000 s | 636 ms | `vehicle_attitude` 与 `vehicle_attitude_setpoint` 四元数误差 |
| 10 秒窗最大下降速度 | 131.360000 s | 1.284 s | `vz=1.983 m/s`，NED 正向向下 |
| 角速度连续超过 5 rad/s | 131.432000 s | 1.356 s | `vehicle_angular_velocity` |
| 本地 `z` 连续越过 0 m 地面平面 | 131.912000 s | 1.836 s | `vehicle_local_position.z` |
| land detector 最终置 landed | 209.472000 s | 79.396 s | `vehicle_land_detected.landed` |

关键采样：

| 相对时间 | M1..M4 命令 | thrust Z | 本地 z / vz |
| --- | --- | ---: | --- |
| T0 | 0.791 / 0.801 / 0.591 / 0.606 | -0.697 | -1.602 m / -0.077 m/s |
| T+0.1 s | 1.000 / 0.667 / 0.616 / 0.608 | -0.728 | -1.599 m / 0.040 m/s |
| T+0.5 s | 1.000 / 0.737 / 0.658 / 0.655 | -1.000 | -1.413 m / 0.780 m/s |
| T+1.0 s | 1.000 / 0.679 / 0.646 / 0.572 | -1.000 | -0.846 m / 1.688 m/s |
| T+2.0 s | 1.000 / 0.775 / 0.673 / 0.410 | -1.000 | 0.023 m / 0.149 m/s |

所以事件顺序为：**注入 → 电机命令触顶 → 分配器饱和/设定值不可实现 → 姿态误差 → 高角速度 → 越过地面平面**。现有 ULog 无法把 confidence、residual、model validity、authority、LOC、impact、recovery 插入这条时间线；下一轮复测必须先证明这些 topic 已记录。

## 0.40 对照

0.40 样本更快进入不可用区：T0=`537.000000 s`，分配器饱和 T+`0.100 s`，设定值不可实现 T+`0.300 s`，下降速度超过 `3 m/s` 为 T+`0.640 s`，本地 z 越过地面平面为 T+`0.984 s`。10 秒窗峰值下降速度为 `6.363 m/s`（T+`1.328 s`）。该样本不适合在 0.70 可观测性通过前用作估计器门限标定。

## 可观测性审计

当前估计器以四路 `actuator_motors.control[]` 原值作为 RLS 回归量，以低通后的角加速度作为响应；激励与置信度却由每路命令相对其慢均值的方差计算。这个合同存在三项风险：

1. 回归量包含共同的 collective thrust 分量，而旋转响应主要由电机差分产生，回归模型和激励判据不在同一信号空间。
2. 四电机命令在悬停闭环中高度相关，单电机效能与共同推力变化可能不可辨识。
3. 估计器不知道 control allocator 已经饱和；饱和后的命令/响应不再代表局部线性、可实现的激励，却仍可更新 RLS 和残差。

0.70 注入前 9.8 秒窗，原命令 Gram 矩阵条件数为 `516`，去均值后为 `11.7`；注入后的短窗分别为 `126` 和 `26.4`。0.40 注入前原命令条件数高达 `3.66e3`，去均值后仍为 `191`。这些数值支持“共同分量和相关性会污染辨识”的风险判断，但由于效能、置信度和残差 topic 缺失，尚不足以选择最终算法修复。

## 已实施的证据链修复

logger 的 FTC 默认 topic 原来全部使用 `add_optional_topic()`。Logger 初始化时大部分 FTC 模块尚未首次发布，`orb_exists()` 失败后这些 topic 被永久排除；`ftc_simulation_status` 和 `ftc_effectiveness_matrix` 因更早发布而恰好存在。

现在 FTC topic 改为固定订阅。Logger 会保留未发布 topic 的订阅槽并周期重试，模块第一次发布后写入 ULog。该改动不改变控制、估计、门限或执行器输出，只修复后续复测的可追溯性。

## 本阶段决策

- 不修改 `FTC_CONF_MIN=0.60`、`FTC_EXC_MIN=0.025`、`FTC_RES_THR=1.0`、`FTC_HLTH_MIN=0.70` 或其他判别门限。
- 不把最后一次估计伪装为“当前可信估计”。下一轮若采用冻结策略，必须同时发布 freshness/validity 降级，旧值只能作为 last-known 诊断。
- 在新 ULog 能同时记录注入、效能、置信度、残差、饱和和飞行状态前，不提交 RLS/滤波/去均值算法改动。
- 0.70 复测应采用更高初始高度、稳定 Position/Loiter、先验证 0.90/0.80 的渐进矩阵；这不是放宽门限，而是避免在 104 ms 内进入饱和后用失控数据标定估计器。
