# 文档审计

审计范围包括根 README、`docs/`、`Documentation/merivus/`、`src/modules/swarm_node/README.md` 及相邻仓库入口。PX4 上游的大量通用 Markdown/CMake 文档保留原结构，不纳入 MERIVUS 内容合并。

## KEEP

| 文档 | 原因 |
| --- | --- |
| `Documentation/merivus/CI_CONTRACT.md` | 产品 CI 和产物同源合同，职责独立 |
| `Documentation/merivus/PIXHAWK_6C_MINI_V6C22.md` | V6C22 硬件身份与验收命令 |
| `Documentation/merivus/RTK_AND_4G_CONFIGURATION.md` | GNSS/4G 唯一配置与迁移合同 |
| `src/modules/swarm_node/README.md` | 与模块源码同放的协议和安全边界 |
| `docs/extreme_control/BASELINE_SNAPSHOT.md` | FTC 开发前不可变恢复锚点 |
| `docs/extreme_control/LOCAL_MODIFICATIONS_AUDIT.md` | 精确记录导入历史和来源不确定性 |
| `docs/extreme_control/SAFETY_GATES.md`、`SITL_TEST_PLAN.md`、`ULOG_ANALYSIS.md` | 专项验证合同，不宜压缩进总架构 |

## MERGE（已执行）

| 原文档 | 规范位置 | 处理 |
| --- | --- | --- |
| `Documentation/merivus/BUILD_AND_FLASH.md` | `docs/development/BUILD_AND_FLASH.md` | 合并有价值步骤；旧路径保留兼容入口，避免双份维护 |
| 根 `README.md` 的分散文档链接 | `docs/README.md` | README 保留快速入口，详细导航统一收口 |

## UPDATE（已执行）

| 文档 | 更新内容 |
| --- | --- |
| 根 `README.md` | 增加系统概览、真实成熟度、FTC 接管边界和统一门户 |
| `docs/extreme_control/README.md` | 补充系统级文档与统一状态入口 |
| `AGENTS.md` | GitNexus、代码来源、核心保护边界和验证规则 |

## MERGE 候选（未执行）

| 文档组 | 原因 | 建议 |
| --- | --- | --- |
| `EFFECTIVENESS_ESTIMATION.md` + `ONLINE_SYSTEM_IDENTIFICATION.md` | 都描述效能/RLS 与模型有效性 | 等 SITL/ULog 证据形成后合并为单一辨识合同 |
| `EMERGENCY_RECOVERY.md` + `RECOVERY_CONTROL.md` | 恢复策略与当前候选实现有部分重叠 | 主动仲裁 ADR 完成后收口 |
| `IMPACT_DETECTION.md` + `LOSS_OF_CONTROL.md` | 共用 `ftc_extreme_state`，但验证场景不同 | 保留到场景测试稳定后再决定 |
| `ARCHITECTURE.md` + 新 `architecture/FTC_ARCHITECTURE.md` | 专项图与系统级图内容相近 | 当前保留：前者是阶段内不变量，后者是全项目入口 |

## ARCHIVE / DELETE_CANDIDATE

- 本轮没有把任何已跟踪技术资料移入 archive，也没有删除候选。
- GitNexus 自动生成的 `CLAUDE.md` 和 `.claude/skills/` 是首次 analyze 的未跟踪工具副本，与精简 `AGENTS.md` 重复，已在提交前移除，不属于历史技术资料。
- `.gitnexus/` 是约 5.0 GiB 的本地数据库、解析缓存和 runner，已由根 `.gitignore` 排除，不作为文档或源码提交。

## 发现的冲突与处理

- 旧构建指南固定写 `git switch main`，不适合所有开发分支。新指南要求记录并构建选定的不可变提交，不替开发者切分支。
- HardwareFMUv6C 仓面向完整板 V6C02，而当前固件主要目标为 Mini V6C22；统一硬件地图明确拆分两个 HW type。
- “控制权限”容易同时指 authority 和 ownership。术语表统一用“剩余控制能力”表示 `ftc_control_authority`，用“控制所有权”表示 setpoint writer。
