# PX4 Extreme Environment / Fault-Tolerant Enhancement

本目录记录 MERIVUS PX4 v1.14 实验性极端环境感知、诊断与容错增强。项目保留现有 EKF2、姿态、角速度、位置控制和 Commander 安全链路；新增功能首先以旁路观察和 shadow mode 运行。

全项目视角从 [文档门户](../README.md) 和 [FTC 系统级架构](../architecture/FTC_ARCHITECTURE.md) 进入；模块验证状态以 [PROJECT_STATUS.md](../PROJECT_STATUS.md) 为统一索引。本目录继续保存算法、安全门、SITL 和 ULog 专项合同。

统一参数前缀为 `FTC_`。所有可能影响执行器或飞行模式的能力默认关闭；当前实现只发布诊断状态和断开的恢复候选，不写入 `actuator_motors`、`actuator_outputs` 或 PX4 正常控制 setpoint。

当前进度：

- G0 Baseline preserved：已完成。
- G1 感知/诊断：`IMPLEMENTED_UNVERIFIED`；消息和参数生成、纯估计器主机编译已验证。
- G2 在线模型：电机效能为 `IMPLEMENTED_UNVERIFIED`，质量/惯量/重心为 `SKELETON`。
- G3-G6 矩阵 shadow、权限、极端状态、恢复候选与 Supervisor：`IMPLEMENTED_UNVERIFIED`，未接管真实控制。
- G7 及后续安全门：未完成，禁止据此开展危险实机试验。

文档索引：

- [BASELINE_SNAPSHOT.md](BASELINE_SNAPSHOT.md)：原工程恢复锚点。
- [LOCAL_MODIFICATIONS_AUDIT.md](LOCAL_MODIFICATIONS_AUDIT.md)：产品修改与来源边界。
- [ARCHITECTURE.md](ARCHITECTURE.md)：数据流、不变量与阶段边界。
- [SAFETY_GATES.md](SAFETY_GATES.md)：逐级验证门槛。
- [PARAMETERS.md](PARAMETERS.md)：统一参数合同和默认值。
- 其余文件分别记录故障诊断、在线辨识、分配、权限、冲击/失控、恢复、Supervisor、SITL 和 ULog 合同。
