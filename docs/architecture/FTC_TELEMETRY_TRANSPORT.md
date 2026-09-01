# FTC 遥测传输

## 范围

本层只把现有 FTC uORB 状态变成版本化 MAVLink 消息。它不修改估计器、控制分配、恢复状态机、Commander、设定值或 `actuator_motors`。

```text
9 个 FTC uORB 主题
  → MavlinkStreamMerivusFtcMotorStatus
  → MavlinkStreamMerivusFtcControlStatus
  → MavlinkStreamMerivusFtcExtremeStatus
  → MavlinkStreamMerivusFtcDiagnostics
  ~> MERIVUS MAVLink 2 方言
  ~> GroundStation VehicleFtcStatusFactGroup
```

## 方言构建

产品核心定义位于：

- `src/modules/mavlink/message_definitions/v1.0/merivus_ftc.xml`
- 包装方言：`src/modules/mavlink/message_definitions/v1.0/merivus.xml`

包装方言先包含上游 `development.xml`，再包含产品定义。`src/modules/mavlink/mavlink` 子模块保持在提交 `18955a04c7c7467e00ea42b704addb4a9c12b53a`，未在子模块内增加文件。CMake 在构建目录暂存上游与产品 XML，然后调用子模块中的生成器。

生成命令固定 `PYTHONHASHSEED=0`。该生成器会把 Python 哈希写入主方言头；固定种子用于保证同一 XML 和工具链产生逐字节相同的头文件。

SITL 和 FMUv6C 使用 `CONFIG_MAVLINK_DIALECT="merivus"`。其他 PX4 板卡未切换方言时不会编译或默认配置这些流。

## uORB 覆盖

| 主题 | 传输用途 |
| --- | --- |
| `motor_health_status` | 电机健康、效能、概率、分类与诊断 |
| `ftc_model_status` | 模型质量、激励与有效标志 |
| `ftc_effectiveness_matrix` | 动态矩阵有效性 |
| `ftc_allocation_shadow` | 影子分配有效性、饱和、偏航牺牲与残差 |
| `ftc_control_authority` | 轴向控制裕度 |
| `ftc_extreme_state` | 撞击、失控和物理诊断 |
| `ftc_recovery_status` | 恢复候选状态、触发、抑制与进度 |
| `ftc_system_status` | 聚合系统状态、置信度和原因 |
| `ftc_simulation_status` | SITL 效能注入状态 |

`motor_health_status.state` 只有在 `STATE_VALID` 时才允许发送电机百分比；其他状态编码为 `255`（不可用），避免把初始化值误报成 0%。

## 默认频率与带宽

| 消息 | 频率 | 未签名流量 |
| --- | ---: | ---: |
| `MERIVUS_FTC_MOTOR_STATUS` | 5 Hz | 450 B/s |
| `MERIVUS_FTC_CONTROL_STATUS` | 5 Hz | 180 B/s |
| `MERIVUS_FTC_EXTREME_STATUS` | 10 Hz | 500 B/s |
| `MERIVUS_FTC_DIAGNOSTICS` | 1 Hz | 77 B/s |

合计约 1,207 B/s。流进入 PX4 标准调度器，可由链路预算和消息间隔命令调整，没有使用常量速率。FMUv6C TELEM1 的 57,600 baud / `MAV_0_RATE=0` 基线下，标称发送预算约 2,880 B/s，必须在台架上与其他消息共同测量。

## 安全语义

`MERIVUS_FTC_CONTROL_STATUS.control_mode` 只按现有数据路径报告：监测关闭为 `DISABLED`，只有观测为 `OBSERVE`，影子分配有效为 `SHADOW`，恢复候选有效为 `CANDIDATE`。当前代码不生成 `ACTIVE`，也不设置 `ACTIVE_COMMAND_PATH`。

`ftc_recovery_status.active` 表示候选状态机处于活动阶段，不表示候选已写入执行器控制链。

## 检查

```bash
python3 Tools/merivus/verify_ftc_telemetry.py
```

脚本检查消息 ID、协议版本字段、9 个 uORB 源、默认频率、两块目标板方言和 ACTIVE 禁令。若同级存在 GroundStation，还会比较两端核心 XML 的 SHA-256。
