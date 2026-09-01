# PX4 Extreme Environment / Fault-Tolerant Enhancement

本目录记录 MERIVUS PX4 v1.14 实验性极端环境感知、诊断与容错增强。项目保留现有 EKF2、姿态、角速度、位置控制和 Commander 安全链路；新增功能首先以旁路观察和 shadow mode 运行。

统一参数前缀为 `FTC_`。所有可能影响执行器或飞行模式的能力必须默认关闭；本阶段不允许写入 `actuator_motors`、`actuator_outputs` 或控制 setpoint。

当前进度：

- G0 Baseline preserved：已完成。
- G1 Motor health monitor compiles and logs：开发中。
- G2 及后续安全门：未完成，禁止据此开展危险实机试验。

文档索引：

- [BASELINE_SNAPSHOT.md](BASELINE_SNAPSHOT.md)：原工程恢复锚点。
- [LOCAL_MODIFICATIONS_AUDIT.md](LOCAL_MODIFICATIONS_AUDIT.md)：产品修改与来源边界。
- [ARCHITECTURE.md](ARCHITECTURE.md)：数据流、不变量与阶段边界。
- [SAFETY_GATES.md](SAFETY_GATES.md)：逐级验证门槛。
- 其余文件分别记录监测、估计、分配、冲击检测、恢复、SITL 和 ULog 合同。

