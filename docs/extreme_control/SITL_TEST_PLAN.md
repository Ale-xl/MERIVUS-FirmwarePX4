# SITL 测试计划

## 故障集合

- 电机效能阶梯：1.0、0.9、0.8、0.6、0.4、0.0。
- 突发、渐变、间歇效能损失。
- 风扰、激烈机动、传感器噪声和冲击脉冲。

## 核心断言

1. 所有 FTC 参数关闭时，执行器输出路径与 baseline 相同。
2. 1.0 效能时 health 接近 nominal，不触发故障。
3. 0.8/0.5/0.0 时按门限和持续时间报告退化/严重/失效。
4. 强风和激烈机动不应被轻易解释为单电机故障。
5. 激励不足或 estimator invalid 时 shadow/intervention 均不生效。
6. shadow topic 可对比 nominal 与候选输出，真实 `actuator_motors` 不被 FTC 发布。

使用原生 MAVLink failure injection 可覆盖完全失效；连续效能需要仅编入 SITL 的最小注入点。任何危险实机故障实验均不属于本计划。

## MAVLink SITL 连续效能注入

连续注入位于 POSIX `simulator_mavlink` 发送 HIL actuator controls 的物理边界，适用于使用该桥接的 jMAVSim/Gazebo Classic。它不改 PX4 内部 `actuator_motors`，因此监测器能对比 nominal command 与退化后的真实仿真响应；直接 GZ bridge 和 SIH 当前不在该注入点覆盖范围内。

```sh
# 电机 2 突降到 50%
param set FTC_SIM_MOT 2
param set FTC_SIM_EFF 0.5
param set FTC_SIM_RAMP 0
param set FTC_SIM_INT 0
param set FTC_SIM_EN 1

# 10 秒渐变到 50%
param set FTC_SIM_RAMP 10

# 每 4 秒在 50% 与 100% 之间切换
param set FTC_SIM_RAMP 0
param set FTC_SIM_INT 4

# 立即恢复 nominal
param set FTC_SIM_EN 0
```

`ftc_simulation_status` 记录目标值、实际斜坡值、电机索引和间歇状态。

## 当前验证状态

- 三个新增 uORB 消息：PX4 v1.14 生成器 header/source 生成通过。
- 17 个 `FTC_` 参数：PX4 参数 JSON 生成通过。
- 连续估计器：主机编译和合成 50% 退化测试通过。
- `px4_sitl_default`：未构建；当前 Windows 主机没有项目要求的 Linux 构建环境。
- 仿真起飞、注入、ULog 和误报场景：未运行，G2 不成立。
