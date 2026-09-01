# MERIVUS 项目状态

## 状态词

`DESIGN` → `SKELETON` → `IMPLEMENTED_UNVERIFIED` → `HOST_VERIFIED` → `BUILD_VERIFIED` → `SITL_VERIFIED` → `HITL_VERIFIED` → `BENCH_VERIFIED` → `FLIGHT_VERIFIED`。`DEPRECATED` 表示退出维护。状态只描述已有证据，不按代码量自动升级。

## 当前模块

| 模块/能力 | 状态 | 已有证据 | 仍缺 |
| --- | --- | --- | --- |
| FMUv6C/V6C22 BMI088 适配 | `IMPLEMENTED_UNVERIFIED` | BSP、HW type、SPI map、启动分流和验收合同 | 当前提交的 L2/L7/L9 |
| Hyper982 NMEA / RTK 配置 | `IMPLEMENTED_UNVERIFIED` | board defaults 与配置合同 | 当前设备的 bench/flight 证据 |
| HyperLte / TELEM1 MAVLink | `IMPLEMENTED_UNVERIFIED` | 57600/Normal/带宽合同与代码修订 | 当前链路 bench/flight 证据 |
| `swarm_node` | `IMPLEMENTED_UNVERIFIED` | 协议、状态机、startup 和 GroundStation 两端源码 | 本分支 L2/L4 及 1/2/6 机逐级验证 |
| `MotorEffectivenessEstimator` 核心 | `HOST_VERIFIED` | 纯估计器主机编译/测试提交 | SITL/ULog/实机数据标定 |
| `motor_health_monitor` 集成 | `IMPLEMENTED_UNVERIFIED` | uORB、参数、门控、fault 分类实现 | L2/L4/L5 |
| 质量在线估计 | `SKELETON` | `ftc_model_status` 字段 | 可观测模型、实现和验证 |
| 在线惯量估计 | `SKELETON` | 标称 `Ixx/Iyy/Izz` 参与观测；`inertia_valid=false` | 在线估计设计与验证 |
| CG 在线估计 | `SKELETON` | `cg_offset` 字段；`cg_valid=false` | 模型、实现和验证 |
| allocator matrix export | `IMPLEMENTED_UNVERIFIED` | `FTC_CA_SHADOW` 只读 hook | L2/L4 与多机架一致性 |
| `ftc_control_monitor` shadow/authority | `IMPLEMENTED_UNVERIFIED` | 动态矩阵、Sequential Desaturation、状态 topic | L3/L4/L5/L6 |
| `ftc_extreme_state_monitor` | `IMPLEMENTED_UNVERIFIED` | impact/hard landing/LOC 状态机 | 分场景 SITL 与 ULog 标定 |
| `ftc_recovery` candidate | `IMPLEMENTED_UNVERIFIED` | 资格门、状态机、断开候选 | L4/L5/L6；主动接管设计 |
| FTC active allocation | `SKELETON` | 仅保留 `FTC_CA_EN` 参数 | 唯一 owner、切换、fallback、全级验证 |
| FTC active recovery arbitration | `SKELETON` | 仅保留 `FTC_REC_ACT`/状态字段 | Commander/failsafe 仲裁和全级验证 |
| `ftc_supervisor` | `IMPLEMENTED_UNVERIFIED` | 聚合五类 topic、顶层状态 | L2/L4/ULog |
| SITL fault injection | `IMPLEMENTED_UNVERIFIED` | simulator hook、5 参数、状态 topic | L2/L4 |
| FTC ULog integration | `IMPLEMENTED_UNVERIFIED` | 9 个 optional topic 已入 logger 源码 | 实际 ULog 出现、时间对齐和带宽 |
| FTC MAVLink telemetry | `IMPLEMENTED_UNVERIFIED` | 版本 1 方言、4 条标准调度流、两仓契约检查 | SITL 编译/频率、FMUv6C 编译、TELEM1 台架和 GroundStation 联调 |
| GitNexus 工程知识图 | `HOST_VERIFIED` | 当前 HEAD 完整索引，无 incomplete reason | 代码变更后需刷新；子模块噪声待优化 |
