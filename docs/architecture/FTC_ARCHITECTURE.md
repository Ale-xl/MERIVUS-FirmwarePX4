# FTC 软件契约

更新：2026-09-09。当前为阶段性软件实现；ACTIVE 路径统一为 `IMPLEMENTED_UNVERIFIED`。主机测试和构建不代表 SITL 飞行、HITL 或实机验证。当前验证证据见 [阶段总结](../testing/FTC_SOFTWARE_STAGE_REPORT.md)。

## 数据链和唯一责任方

`actuator_motors + IMU + nominal B + allocator status → motor_health_monitor → model/diagnosis → ftc_control_monitor → authority/shadow`。

`impact/LOC + health + authority → ftc_recovery → ftc_recovery_status → FtcRateInput/FtcRecoveryArbiter → mc_rate_control 正常 torque/thrust → control_allocator → 原生输出链`。

`model λ → FtcAllocationPolicy → nominal B 按列缩放 → 原生 Sequential Desaturation → 实际分配`。

实际所有权由 `ftc_allocation_status` 和 `ftc_arbitration_status` 反馈。Supervisor 只聚合事实，不写控制。FTC 不竞争发布 `vehicle_rates_setpoint`，不直接发布电机/PWM。Commander、EKF2、位置和姿态控制核心没有改动；角速度控制只增加局部输入选择，分配器只在自身层应用动态矩阵。

## 估计器

`baseline_learned`、`current_observable`、`estimate_valid`、`estimate_uncertainty`、`estimate_age`、`last_valid_timestamp` 和 `excitation` 分别发布。状态枚举包括 UNINITIALIZED、CALIBRATING、BASELINE_LEARNED、OBSERVABLE、TEMPORARILY_UNOBSERVABLE、VALID、STALE、INVALID；字段是事实，状态是本周期主要原因。

命令和角响应使用相同低通与时间均值去除。回归量为 `phi[a][i] = B_nominal[a][i] * du[i]`；基线学习轴响应比例，之后联合估计每电机一个 λ，避免三轴自由系数范数比混淆。基线假设初始动力健康，不能识别训练前已有的共同损伤。

激励是差分命令 RMS 相对于 `FTC_EXC_MIN` 的比值。滚动信息矩阵最小特征值大于 1e-6、条件数小于 1000，并满足当前激励与输入门时才可继续学习。相关 collective 输入的秩退化会阻止虚假高置信度。12 电机固定容量；条件数每十个样本做固定八次 Jacobi 扫描，工作数组保存在对象中。H743 最坏 CPU/栈仍需目标测量。

RLS 协方差 P 表达 λ 不确定度；加入 `0.0004 λ²/s` 随机游走，响应噪声下限 `0.05 rad/s²`，残差过大扩大不确定度。`confidence = exp(-0.5*(sigma/0.15)^2)` 是质量分数，**未经概率校准**。低激励不会直接把 confidence 置零，但不确定度会增长、年龄会增加。默认 10 s 无更新成为 STALE；数值/输入错误成为 INVALID。有效性另受 confidence、残差和年龄约束。历史值保留，不能当作实时健康证据。

命令环形窗口 32 帧，使用 actuator 发布时刻，按 IMU 响应样本时刻减 `FTC_EST_DELAY`（默认 40 ms）选取零阶保持命令；超过 40 ms 空隙拒绝匹配。同一 IMU 响应不重复累计信息。改变延迟/低通、机架几何、会话复位或时间回退会清除相应学习状态。固定延迟仍待不同机型标定。

allocator 饱和、headroom 和控制残差形成独立门，暂停更新而保留历史基线/年龄/不确定度。预测残差是预测和实测角响应之差；`rigid_body_activity` 独立记录 `||I*alpha + omega×Iomega||`，不冒充预测残差。

## 诊断与刚体观测

每电机诊断状态区分 NO_EVIDENCE、UNKNOWN、UNOBSERVABLE、VALID_HEALTHY、DEGRADED、FAILED。有效性不足不等于健康。效能持续越过健康/失效门限形成掩码；分类分数是启发式证据强度，仍待混淆矩阵与阈值标定。ESC online/failure/RPM 为可选辅助，缺少 ESC 不阻止效能估计；电流/电压/温度未实现独立故障阈值模型。

Mass 为 observe-only：只有 `FTC_THR_MAX > 0` 提供独立、可信的总推力尺度，且姿态/运动门满足时，才累计质量估计；默认尺度 0，输出不可观测。依赖同类电机线性推力尺度近似，不能把归一化控制量当作牛顿。惯量和 CG 的接口、状态、日志齐备，在线估计仍为 NOT_IMPLEMENTED/NOT_VALID；标称惯量只用于 activity。三者均不反馈主动控制。

## 主动分配

`FTC_MON_EN && FTC_CA_EN` 是请求，不代表已介入。还要求 armed、非 failsafe、非 VTOL、单矩阵普通多旋翼、4–12 电机、无舵机/可逆电机、无原生失效移除、Sequential Desaturation/AUTO、模型和 authority 新鲜有效。模型年龄最多 2 s、单电机 sigma 不大于 0.12、RP 权限至少 0.35、推力至少 0.25。λ 小于 0.95 持续 1 s 才介入，保持门为 0.98；估计器复位重新等待。

动态矩阵始终从 nominal B 构造，保持原生归一化。λ 介入限速 0.2/s，普通关闭/失效/超时按 1/s 回 nominal；几何改变、不支持结构、解除武装和 failsafe 等硬门立即恢复 nominal。没有新力矩指令的备份调度周期也更新回退状态，但不额外发布旧控制指令。FTC 参数独立刷新，避免飞行中关闭请求被原生配置的落地更新门阻挡。

偏航权限低于 0.2 时平滑降低 yaw 请求权重，高于 0.3 恢复，使用 v1.14 原生 sequential 优先策略。方向正/负权限、上下推力余量和 reachable residual 单独发布；这些轴投影不是多轴同时可达的数学保证。

## 恢复和重入

顺序：MONITORING → DISTURBANCE_DETECTED → RATE_DAMPING → THRUST_VECTOR_RECOVERY → ATTITUDE_RECOVERY → VERTICAL_SPEED_RECOVERY → ALTITUDE_STABILIZATION → CONTROL_REENTRY。可转 EMERGENCY_LAND、ABORTED 或 FAILED。

角速度阻尼优先 p/q，yaw 限幅较小，四元数恢复 body-Z；设定值带幅值和 slew 限制。姿态恢复后才进入垂直速度/高度闭环，使用 NED 垂直速度、悬停推力和倾斜补偿，带积分限幅。位置不可维持但姿态/垂直状态可用时生成 0.7 m/s 受控下降；RP/推力不足、姿态仍倒置且超时则 FAILED，不强行垂直控制。总恢复超时 20 s；终止状态在飞行会话内保留。

唯一仲裁位于 rate controller 输入，正常设定值继续由 PX4 维护。正常/candidate 200 ms 新鲜度、飞行模式、armed/land/failsafe/termination、authority 和候选数值边界都参与检查。介入权重最高 1/s、普通退出 2/s；模式改变和安全硬退出立即归还原生路径。默认关闭且未曾介入时输出与正常输入一致。

重入要求 rate、姿态、垂直速度、authority 持续稳定，并且正常与恢复 rate 差小于 0.5 rad/s、推力差小于 0.2。稳定超过 0.5 s 后请求权重在 2 s 内降至 0，反馈确认零权重后回 MONITORING。状态反复恶化时回到阻尼或终止；不切换 Commander 模式。

## 模式能力矩阵

| 场景 | 观察/Shadow | 主动分配 | 主动恢复 |
| --- | --- | --- | --- |
| SITL/FMuv6C 普通单矩阵多旋翼 | 已实现 | 默认关闭，有门控 | 默认关闭，有门控 |
| POSCTL / ALTCTL / STAB / AUTO_LOITER | 已实现 | 符合分配门时支持 | 支持原模式内仲裁 |
| ACRO / OFFBOARD / MISSION / RTL | 可观察 | 符合分配门时支持 | 不支持，走原生控制 |
| VTOL / 多矩阵 / 舵机 / 可逆电机 | 不保证效能辨识 | 不支持，nominal | 不支持 |
| 无 ESC 数据 | IMU/control 模式 | 不依赖 ESC | 不依赖 ESC |
| 多机主/从角色 | 每机独立观察 | 本机门控，无编队权限 | OFFBOARD 从机不接管 |

## 状态、遥测和日志

Supervisor 区分关闭、初始化、学习、READY、不可观测、正常、退化、确认故障、SHADOW、CANDIDATE、ACTIVE_ALLOCATION、ACTIVE_RECOVERY、EMERGENCY_LAND、FAILED。模型无效不报告绿色 NORMAL。ACTIVE 必须来自分配器/仲裁器的实际权重反馈，参数置 1 或存在候选都不等于 ACTIVE。

MAVLink v2 契约见 [遥测传输](FTC_TELEMETRY_TRANSPORT.md)。新增 `ftc_allocation_status`、`ftc_arbitration_status`，其余模型、诊断、authority、恢复和系统 topic 扩展。固定 logger 订阅包括基线/年龄/不确定度/对齐/更新/饱和/实际权重/回退原因。新日志字段实际记录与传输时延仍待统一仿真验证。

所有危险默认值继续为 `FTC_CA_EN=0`、`FTC_REC_ACT=0`、`FTC_SIM_EN=0`。本阶段不运行新的故障飞行、不启动 HITL/台架/实机。软件实现不作为启用 ACTIVE 的验证依据。
