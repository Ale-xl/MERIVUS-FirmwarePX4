# FTC 正常飞行置信度衰减分析

日期：2026-09-08。范围：用户提供的六张截图、本地源码、通过 `ssh px4vm` 读取的 SITL 状态与 ULog。此次只分析和记录证据，没有修改算法、参数或飞行状态，也没有启动新飞行或故障注入。

## 结论

停止后续故障注入的决定正确，但“estimator 从未建立正常 Baseline”与实际日志不符。与截图匹配的会话在 PX4 时间 **540.640 s** 首次进入 `VALID`，四电机 confidence 均曾达到 **1.0**。551.460 s 变为 `INVALID`，随后有两段短暂恢复，590.600 s 起直到本次检查的 1000 s 都无有效模型。

直接原因是当前实现把**近期电机命令的激励强度**当作 **confidence**。机动结束后，命令波动减小，confidence 随约 2 s 时间常数的统计量衰减；即便内部 Baseline 已建立，也会因为四电机 confidence 不再全部达到 0.60 而使模型失效。这符合现有代码，却不能满足“机动学习后，在稳定悬停中持续提供可信电机效能”的测试预期。

另外存在两个显示问题：Supervisor 在模型无效时仍可发布 `NORMAL`；地面站把数值不可用统称为“数据已过期”。因此截图上的绿色“正常”不能用作 Baseline 通过依据，“数据已过期”也不能单凭文字认定为链路丢包。

## 证据身份与范围

- FirmwarePX4：`research/extreme-control-v1`，HEAD `e2d59728b93b5bf3aa300ac175373fa75c255f58`，本地与虚拟机都有既有未提交修改。此次未覆盖这些修改。
- GroundStation 本地源码：`research/groundstation-ftc-ui`，HEAD `1d49372`，工作树原本干净。未核对截图所用地面站二进制的内容哈希，界面根因以本地代码与截图行为的一致性为依据。
- `px4-ver all` 返回 `PX4_SITL`、相同固件 HEAD、构建时间 `Sep 3 2026 01:42:00`、GNU GCC 9.4.0。HEAD/构建信息不证明带未提交修改的二进制与当前源码逐字节同源。
- 已核对本地与虚拟机关键源码 SHA-256 完全一致：

| 文件 | SHA-256 |
| --- | --- |
| `src/modules/motor_health_monitor/MotorEffectivenessEstimator.cpp` | `2efd145dbba6ea099860f7b40719b53af2d6579d3d96e5666b7cbe42330caa48` |
| `src/modules/motor_health_monitor/MotorHealthMonitor.cpp` | `a4b013966e82386ed01e21fa7035d40430fc38b641c0cd3b28b48c1be659f333` |

匹配截图的日志为：

```text
/home/cwkj/MERIVUS/FirmwarePX4/build/px4_sitl_default/rootfs/log/2026-09-08/06_03_24.ulg
```

通过 Python 标准库按 ULog 内嵌格式定义解码选定 topic，读取至首个选定 topic 时间超过 1000 s 的记录。本次读取前缀 **259162916 字节**，SHA-256：

```text
6d6df8c108e9967c0676822cf660a3cfa990002a9a87d17f111066caf81d7ee3
```

该哈希只标识已分析前缀，不是仍在增长的整份日志的哈希。可重复核对：

```sh
head -c 259162916 /home/cwkj/MERIVUS/FirmwarePX4/build/px4_sitl_default/rootfs/log/2026-09-08/06_03_24.ulg | sha256sum
```

669.020 s 的 `ftc_model_status`、`motor_health_status`、`ftc_control_authority` 与截图中的时间戳和数值一致，包括四路 confidence、model residual 和 headroom。因此可以绑定截图中的异常时刻。截图一出现的旧日志 `2026-09-07/09_03_37.ulg` 也检查了前 750 s，但同一时间点仍在地面且 confidence 全零，与空中截图不符，不能把两段会话直接拼接。

匹配日志前缀中有六个核心 FTC 状态 topic 和仿真状态 topic；`ftc_simulation_status` 共 49975 条、`enabled=true` 为 0 条。所读参数为 `FTC_MON_EN=1`、`FTC_CA_SHADOW=1`、`FTC_CA_EN=0`、`FTC_REC_ACT=0`、`FTC_SIM_EN=0`、`FTC_SIM_EFF=1.0`，以及 `FTC_EXC_MIN=0.025`、`FTC_CONF_MIN=0.60`、`FTC_BASE_T=5.0`、`FTC_LPF_TC=0.20`、`FTC_RES_THR=1.0`。

## 实际状态时间线

时间均为该次启动的 PX4 单调时间，不是截图右上角的日期时间。

| 时间 / s | 原始状态 | 意义 |
| ---: | --- | --- |
| 533.316 | `vehicle_status.arming_state=2` | 解锁 |
| 535.084 | `vehicle_land_detected.landed=false` | 离地 |
| 535.100 | `motor_health_status.state=1` | 进入 `CALIBRATING` |
| 540.640 | `state=2`、`model_valid=true` | 首次有效，证明内部 Baseline 已建立 |
| 551.460 | `state=3` | 首次退出有效状态 |
| 586.500 | `state=2` | 再次有效 |
| 588.340 | `state=3` | 再次无效 |
| 588.760 | `state=2` | 再次有效 |
| 590.600 | `state=3` | 此后到 1000 s 均未恢复有效 |
| 669.020 | confidence 为 `0.0270634 / 0.0251525 / 0.0235597 / 0.0256287` | 与用户截图一致 |

`ftc_model_status` 与 `motor_health_status` 各有 725 条有效记录，有效采样覆盖约 14.5 s，分布于三段，不能把首次至最后一次有效之间的约 50 s 全算作连续有效。四电机 confidence 的各自最大值均为 1.0。600–675 s 四路最小 confidence 的最大值只有 0.15437；675–1000 s 只有 0.041446。

SSH 当前 listener 也复现同类现象：1848.860 s，四路 confidence 为约 `0.01945 / 0.01971 / 0.01764 / 0.01984`，`state=3`、`model_valid=false`，模型残差 0.04904。这个当前快照用作复现证据，不与 1000 s 前缀混算。

## 代码根因

### 1. confidence 不是累计学习可信度

责任代码：[MotorEffectivenessEstimator.cpp](../../src/modules/motor_health_monitor/MotorEffectivenessEstimator.cpp)，`update()`。

输入先经过默认 0.20 s 低通；均值与方差使用 `statistics_alpha = dt / (2 + dt)`。每路输出为：

```text
confidence[i] = clamp(sqrt(control_variance[i]) / FTC_EXC_MIN, 0, 1)
excitation = clamp(sqrt(sum(control_variance) / motor_count) / FTC_EXC_MIN, 0, 1)
```

所以截图的 `excitation=0.02538` 已经除过门限，不能据此认为它“大于 FTC_EXC_MIN=0.025，已经足够激励”。其对应的命令标准差约为 `0.02538 × 0.025 = 0.0006345`，即归一化满量程的约 **0.06345%**。每路 confidence 达到 0.60 则需要对应标准差至少为 **0.015**，即满量程的 1.5%。

飞行时间不会进入该 confidence 公式，RLS 协方差、累计有效样本量和独立可观测性也没有进入。稳定悬停降低激励，等待更久不会使 confidence 自动升至 0.60。ESC 数据虽然存在，也未参与这个估计器的 confidence 计算。

### 2. Baseline 建立与当前 model valid 是两件事

只有平均命令方差严格超过 `FTC_EXC_MIN²`，RLS 才更新，`_baseline_elapsed` 才累计；它不是起飞后简单计时 5 s。累计达到 `FTC_BASE_T` 且每路系数范数及 confidence 满足初始条件后，内部 `_baseline_valid` 置 true。

[MotorHealthMonitor.cpp](../../src/modules/motor_health_monitor/MotorHealthMonitor.cpp) 的当前有效条件为：

```text
flight_valid
&& estimate.baseline_valid
&& 所有电机 confidence >= FTC_CONF_MIN
&& estimate.model_residual <= FTC_RES_THR
```

Baseline 后低激励不会清除内部 `_baseline_valid`，但会使当前模型 invalid。`state=1` 才是有效飞行条件下 Baseline 未完成的 `CALIBRATING`；截图为 `state=3` 且 confidence 非零，与“已学过、当前信心不足”的路径一致，日志进一步证明了这一点。

669.020 s 模型残差只有 **0.0409202**，低于门限 1.0。此时四路 confidence 都远低于 0.60，是明确失败条件。`model_quality=0` 是模型 invalid 时发布代码直接给出的值，并不是另一项独立的质量测量归零。

### 3. 下游 invalid 是依赖传播

- [FtcControlMonitor.cpp](../../src/modules/ftc_control_monitor/FtcControlMonitor.cpp) 将 `shadow.valid` 和 `authority.valid` 直接取自 `_health.model_valid`。所以矩阵有效、名义四轴 authority 都为 1、headroom 为 0.587268、饱和掩码为 0，也仍可 `authority.valid=false`。名义裕度不能证明无效效能模型可信。
- [FtcRecovery.cpp](../../src/modules/ftc_recovery/FtcRecovery.cpp) 的 `inhibit_reason_mask=64=0x40` 对应 `INHIBIT_AUTHORITY`。这是 authority 无效导致不可进入恢复候选；截图 `state=1` 为 `MONITORING`，没有触发恢复，不是状态机卡死。
- [FtcSupervisor.cpp](../../src/modules/ftc_supervisor/FtcSupervisor.cpp) 中 `reason_mask=1` 对应模型无效；系统 confidence 为模型、authority、extreme 三项的平均值。当前 `(0+0+1)/3=0.33333`，不是电机估计器获得了 33% 的可信度。
- `degraded_mask=failed_mask=0` 表示当前没有确认这些故障；模型无效时，效能阈值故障判别被门控，不能用零掩码证明电机效能已被可靠验证。

### 4. 地面站文案混淆了状态

[FtcSupervisor.cpp](../../src/modules/ftc_supervisor/FtcSupervisor.cpp) 默认设置 `NORMAL`，在 `model.valid=false` 分支只设置原因位，不改变 state。地面站 [VehicleFtcStatusFactGroup.cc](../../../GroundStation/src/Vehicle/VehicleFtcStatusFactGroup.cc) 根据该枚举直接显示“正常”和绿色；authority 文本也可根据状态枚举显示“控制裕度完整”，同时数值因 validity 为 false 显示 N/A。显示的枚举描述缺少有效性条件。

[MERIVUS_FTC_MOTOR_STATUS.hpp](../../src/modules/mavlink/streams/MERIVUS_FTC_MOTOR_STATUS.hpp) 对 `motor.state != STATE_VALID` 的电机把健康、效能、故障概率和 confidence 全编码为不可用值。地面站行模型收到这些值后设置 `available=false`；[FtcStatusPanel.qml](../../../GroundStation/custom/res/Merivus/FtcStatusPanel.qml) 将任何 `available=false` 都显示成“数据已过期”。

因此，在消息持续更新但模型无效时，也能得到截图中的 N/A 和“数据已过期”。截图不足以独立排除链路过期，但现有代码已经存在一条完整的误标路径，不应先把问题归因于 UDP、刷新率或重启。

### 5. 其他字段的正确解释

- 四电机之外的效能/健康 NaN 是未使用数组槽位；有效电机数量为 4。
- `mass`、`cg_offset` 的 NaN 和对应 validity=false 是当前未实现估计的显式输出；惯量只是参数值，`inertia_valid=false`。这些不参与当前 `model.valid` 判定，不是本次低 confidence 的触发原因。
- `rotational_residual=0.00714` 实际由 `norm(I × angular_acceleration + omega × (I × omega))` 计算，没有减去预测力矩；它不能当成 RLS 模型拟合残差。当前有效门使用 `motor_health_status.model_residual`。
- `impact_detected=false`、`loc_state=0`、`loss_of_control_score=0` 与正常飞行一致。impact score 约 0.117 不代表检测到了撞击。
- `FTC_CA_SHADOW=1` 表示开启影子计算入口；模型 invalid 时 `shadow.valid=false`，MAVLink 模式会报告 `OBSERVE`。截图显示 OBSERVE 不等于参数没有生效。

## 修复边界与验证要求

本次已经定位直接原因；尚未证明当前 RLS 在真实闭环数据上给出的效能比例具有足够精度，不能把“曾经 valid”表述为健康基线辨识质量已经通过验证。

后续应在估计器接口与状态契约处区分：Baseline 是否完成、当前激励是否足够、估计不确定度、最后有效更新时间、当前输出是否仍可信。confidence 应有可解释的辨识质量依据，例如有效信息矩阵、条件数或协方差，并考虑样本老化；不能通过降低 0.60 门限、强制保持 valid 或只显示历史值来完成修复。低激励时如没有证据证明当前效能仍可信，应明确报告不可观测/不可用。

同时评估当前原始电机命令回归量中的共同推力分量、四路相关性、命令与角加速度时间对齐，以及缺少 allocator 饱和输入门的问题。这些是后续算法验证项，不应把它们全当作此次快照已经证明的独立故障。

状态层应把“监测就绪”与“没有确认故障”分开；地面站应分别表达未接收、消息过期、模型学习中、模型无效和有效数据，保留可解释的诊断 confidence。数值与文本都应遵守同一 validity。

现有 [MotorEffectivenessEstimatorTest.cpp](../../src/modules/motor_health_monitor/MotorEffectivenessEstimatorTest.cpp) 使用持续独立正弦激励，并把 LPF、激励门限、Baseline 时间改为 0.03 s、0.01、2 s；它没有覆盖默认参数下“起飞/机动→长时间悬停”的状态生命周期。修复至少需要：

1. 使用默认参数与该日志复现此次有效→无效序列，并验证状态原因和样本新鲜度。
2. 持续低激励、相关输入和饱和条件下不能输出虚假的高置信度。
3. Baseline 后转入悬停时，当前值与历史值的 validity/age 可解释，不能只靠延长等待通过门限。
4. 模型 invalid 但 MAVLink 新鲜时，地面站显示模型不可用；真正超时才显示数据过期，且不再显示绿色就绪。
5. 正常飞行验证通过并证明 FTC 日志完整之后，再按已有复测矩阵进入独立故障注入场景。

此次已执行源码条件核对、关键源码哈希比对、SSH 只读状态检查、匹配 ULog 前缀解码和 `git diff --check`。未运行完整构建、单元测试或新的 SITL 飞行：此次没有算法/配置变更，已有运行实例和原始日志足以确认直接原因；修复后的飞行效果尚未验证。
