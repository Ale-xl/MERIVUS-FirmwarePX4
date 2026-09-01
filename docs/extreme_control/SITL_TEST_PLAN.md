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

