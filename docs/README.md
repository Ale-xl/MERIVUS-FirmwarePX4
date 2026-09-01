# MERIVUS 文档门户

本目录是 FirmwarePX4 产品文档的统一入口。PX4 上游通用资料继续位于 `Documentation/`；MERIVUS 的当前架构、产品合同、实验状态和开发规则以本页链接为准。

## 1. Project Overview

- [仓库 README](../README.md)：项目、支持范围、快速构建和安全边界
- [当前模块状态](PROJECT_STATUS.md)：统一证据状态，不把实现等同于验证
- [文档审计](DOCUMENT_AUDIT.md)：旧文档的 KEEP/MERGE/UPDATE/ARCHIVE 分类

## 2. Architecture

- [系统地图](architecture/SYSTEM_MAP.md)
- [代码来源地图](architecture/CODE_OWNERSHIP_MAP.md)
- [关键数据流](architecture/DATA_FLOWS.md)
- [关键调用链](architecture/CALL_CHAINS.md)
- [FTC 架构](architecture/FTC_ARCHITECTURE.md)

## 3. Hardware

- [硬件—软件对应关系](hardware/HARDWARE_SOFTWARE_MAP.md)
- [Pixhawk 6C Mini V6C22 合同](../Documentation/merivus/PIXHAWK_6C_MINI_V6C22.md)
- 相邻硬件仓：`E:\MERIVUS\HardwareFMUv6C`（独立 Git 仓库）

## 4. Flight Stack

- 主控制链见 [关键数据流 Flow 1](architecture/DATA_FLOWS.md#flow-1飞行控制与执行器)
- PX4 原生与产品修改边界见 [代码来源地图](architecture/CODE_OWNERSHIP_MAP.md)

## 5. Communication

- [RTK 与 4G 配置合同](../Documentation/merivus/RTK_AND_4G_CONFIGURATION.md)
- [构建与 CI 产物合同](../Documentation/merivus/CI_CONTRACT.md)

## 6. GNSS / RTK

- [GNSS/RTK 数据流](architecture/DATA_FLOWS.md#flow-5hyper982-gnss--rtk-到地面站)
- [参数索引](reference/PARAMETER_INDEX.md#fmuv6c-产品默认参数)

## 7. Swarm

- [机载 `swarm_node` 协议](../src/modules/swarm_node/README.md)
- [编队事务数据流](architecture/DATA_FLOWS.md#flow-6编队事务与位置租约)
- 配套实现：`E:\MERIVUS\GroundStation\custom\src\Swarm\SwarmController.*`

## 8. FTC / Extreme Control

- [FTC 系统级架构与接管边界](architecture/FTC_ARCHITECTURE.md)
- [FTC 专项文档索引](extreme_control/README.md)
- [FTC 参数](reference/PARAMETER_INDEX.md#ftc-参数)
- [FTC uORB](reference/UORB_INDEX.md#ftc-消息)

## 9. Development

- [构建与刷写](development/BUILD_AND_FLASH.md)
- [修改影响分析](development/CHANGE_IMPACT_GUIDE.md)
- [后续重构候选](development/REFACTOR_CANDIDATES.md)

## 10. Testing

- [L0–L9 测试矩阵](testing/TEST_MATRIX.md)
- [FTC SITL 专项计划](extreme_control/SITL_TEST_PLAN.md)
- [FTC ULog 合同](extreme_control/ULOG_ANALYSIS.md)

## 11. Reference

- [参数索引](reference/PARAMETER_INDEX.md)
- [uORB/内部消息索引](reference/UORB_INDEX.md)
- [统一术语表](reference/GLOSSARY.md)

## 12. Historical / Audit

- [FTC 开发前基线快照](extreme_control/BASELINE_SNAPSHOT.md)
- [本地修改来源审计](extreme_control/LOCAL_MODIFICATIONS_AUDIT.md)
- [FTC 变更记录](extreme_control/CHANGELOG.md)

文档结论与源码冲突时以当前提交源码为准，并在同一变更中修正文档。状态升级必须提供对应测试等级的可复现证据。
