# FTC 软件阶段总结（2026-09-09）

按用户“取得阶段性成果就停”的最新要求，本轮停在核心代码路径、主机测试、SITL 构建和地面站构建收口。**这不是整套 FTC 飞行验证完成报告。** 当前主动能力均为 `IMPLEMENTED_UNVERIFIED`，默认关闭。

## A. Git

两仓分支：`codex/ftc-software-closure`。本次验证的代码提交：

- Firmware：`e6c5479fd5753ab8f6ce49e7f888c4944b40c581`。
- GroundStation：`20d36a9601f116caefc88e284d456efdf4f48d67`。
- 此后仅有本阶段说明文档提交；最终 HEAD 以各仓 `git rev-parse HEAD` 为准，构建证据绑定上面不可变代码提交，不混用文档 HEAD。
- 原有修改先分别提交为 `f3067d310d`（构建兼容）和 `f6741bff29`（日志/会话复位及既有证据），未与新算法开发混在一起。
- 仓库外保护目录：`E:/MERIVUS-ftc-backup-20260908`；原 patch SHA-256 `60cbaf7cccfb65c8b62540a703a193877045c2f6e306804862622899b13eb003`，与 VM 既有保护 patch 一致。
- 没有 push、merge、强制重置，也没有替换正在运行的 SITL 或地面站。固件构建使用独立 detached worktree `/home/cwkj/MERIVUS/FirmwarePX4-ftc-closure`。

GitNexus impact 已重试；对核心 Run/update 和 GS 接收/显示符号得到部分 UNKNOWN、lower-bound 结果，索引存在旧版本及运行时边缺失。提交前 detect_changes 已执行。**记录为 GITNEXUS_PARTIAL/UNKNOWN，不是 PASS，也不是本轮外部下载阻塞。** 用 uORB、核心调用点、CMake、启动配置和 diff 补充审计；零 caller/affected count 不等于零风险。

固件功能提交：

```text
e6c5479fd5 fix(ftc): 避免重复响应样本并同步主动参数说明
a38f3a53f5 feat(mavlink): 升级 FTC v2 估计质量与实际介入遥测
b87165acfa fix(ftc): 收紧恢复边界并补充重入与回退契约测试
6564564477 fix(control-allocator): 完善 FTC 超时与结构变化回退
2455045666 feat(mc-rate-control): 接入唯一 FTC 恢复输入仲裁
c48448c3ae feat(ftc): 实现恢复阶段与状态仲裁契约及离线测试
29fbcf8fa8 feat(control-allocator): 接入带安全门与平滑回退的 FTC 动态矩阵
f252453104 feat(ftc): 分离可观测性与置信度并实现对齐和样本老化
fb71b2030a feat(ftc): 建立估计质量与主动路径状态消息契约
f6741bff29 fix(ftc): 固化此前日志订阅与会话复位修复及测试证据
f3067d310d fix(build): 固化此前 PX4 构建兼容修复
```

## B. Estimator

改为 nominal 几何约束下的联合 λ RLS，拆分基线、当前可观测性、有效性、年龄、最后有效时间和不确定度。学习后的低激励不再等于“从未建立基线”。输入/几何/时钟异常有明确复位，重复 IMU 响应不重复累计信息。

## C. Confidence

由协方差和预测残差得到 sigma，再映射为质量分数；加入过程噪声使无新信息期间的不确定度增长。该分数尚未做统计概率标定，不能称为真实故障概率。

## D. Observability

使用差分命令、nominal torque 列和滚动信息矩阵条件判断。低激励或输入相关导致的秩退化暂停学习；保留 baseline 与历史值并显示年龄。

## E. Saturation

接入 allocator saturation、headroom 和 control residual。饱和/控制受限时暂停更新，保留历史估计并老化；不永久抹去基线。

## F. Timing

32 帧命令窗口，以 actuator 发布时刻匹配 IMU 样本减固定延迟，默认 40 ms；空隙过大拒绝对齐。延迟和低通变化复位。不同仿真/硬件实际延迟尚未标定。

## G. Dynamic Allocation

`λ → nominal B 列缩放 → v1.14 Sequential Desaturation → 实际分配` 已有软件路径。包含新鲜度、sigma、authority、持续时间、复位及机型门，λ 限速、滞回和 yaw 权重调整。

## H. Fallback

普通关闭/模型失效/超时平滑回 nominal；结构变化、解除武装、failsafe 等硬门立即归还原生分配。即使没有新力矩消息，也处理 FTC 超时回退。`FTC_CA_EN` 可独立刷新，避免飞行中关闭请求受原生配置落地门限制。

## I. Recovery

实现阻尼、body-Z 四元数恢复、姿态恢复、垂直速度恢复、高度稳定、受控下降及 ABORTED/FAILED。恢复候选仅有一个 rate 输入仲裁消费者；不竞争发布原生设定值，不写原始电机输出。Commander 不改动。

## J. Re-entry

要求角速度/姿态/垂直速度/authority 持续稳定，并检查正常设定值与恢复设定值的差异；满足后渐变权重、确认实际反馈再回 MONITORING。倒置超时不能直接转垂直下降，模式/安全状态变化会退出。

## K. Supervisor

区分初始化、学习、READY、不可观测、正常、退化、故障、Shadow、Candidate 和实际 Active。实际介入依据 allocator/arbiter 反馈；模型无效不再显示绿色正常。

## L. MAVLink

契约升级到 v2，原 ID/基础 MIN_LEN/CRC 不变，扩展不确定度、年龄、诊断状态、方向权限、实际介入/回退/权重和刚体状态。两仓 XML 一致；最大未签名流量约 1,904 B/s，签名后 2,177 B/s，链路测量待后续。

## M. GroundStation

区分消息未收到、消息过期、学习中、当前不可观测、历史估计过期、模型不可用、有效、退化/故障。主 H/E 按模型有效性显示；Tooltip 增加 sigma/age，内部诊断进详情。实际 UI 回放、断链恢复和新协议端到端测试未执行。

## N. Mass / Inertia / CG

Mass observe-only 已实现，但必须提供独立可信的总推力尺度；默认 `FTC_THR_MAX=0`，保持不可观测。惯量/CG 只有完整接口、状态与日志，在线估计仍 NOT_IMPLEMENTED/NOT_VALID；没有假有效值，也不参与主动控制。

## O. uORB / Logging

新增 `FtcAllocationStatus.msg`、`FtcArbitrationStatus.msg`；扩展模型、健康、authority、recovery、system 消息，更新生成清单和固定 logger 订阅。新字段实际 ULog 运行采集仍待下一阶段。

## P. Parameters

新增 `FTC_EST_DELAY=0.04 s`、`FTC_EST_AGE=10 s`、`FTC_THR_MAX=0 N`；同步主动参数描述。源代码默认 `FTC_CA_EN=0`、`FTC_REC_ACT=0`、`FTC_SIM_EN=0`。没有修改 VM 当前运行参数。

## Q. Tests

| 检查 | 本轮结果 | 证据范围 |
| --- | --- | --- |
| Estimator/Alignment/Mass 主机测试 | 7/7 PASS | 默认生命周期、低激励、秩退化、饱和、延迟、λ=1/.9/.8/.7 合成真值、质量尺度/过期 |
| Allocation/Recovery/Arbiter 主机测试 | 10/10 PASS | 持续故障、安全门、回退、模式/数值边界、状态顺序、下降、倒置超时、设定值匹配 |
| `make px4_sitl_default -j4` | PASS | 上述固件代码提交，未启动飞行 |
| GroundStation Release | PASS | 上述地面站代码提交；Qt 5.15.2，VS2022 工具链 |
| Firmware 遥测检查 | PASS | XML、uORB 源、配置/发送契约 |
| GS 遥测静态检查与 mavgen `-Check` | PASS | typed backend/QML 契约、锁定生成器与生成头一致 |
| 两仓 `git diff --check` | PASS | 无空白错误 |
| 新版本 FMUv6C 构建 | 未执行 | 按阶段收口；旧版本通过记录不冒充本版本 |
| 新故障飞行/HITL/台架/实机 | 未执行 | 用户本轮明确不执行 |

固件首轮严格编译发现浮点直接比较警告，改为容差比较后通过。GS 链接有 Qt 静态库缺少 PDB 的非致命警告，Release 构建成功。

主机编译使用 g++ 9.4.0、C++14、`-O2 -Wall -Wextra -Werror` 与系统 gtest；SITL 工具为 CMake 3.16.3、Ninja 1.10.0。固件依赖使用仓库 gitlink 锁定提交，从已有本地对象初始化隔离工作树；未在 VM 原源码目录直接修补。

## R. Current status

本阶段达成：估计/诊断、Shadow、方向权限、Recovery 控制律/重入、Supervisor、v2 遥测和 GS 状态的软件路径；纯核心有 HOST_TEST_VERIFIED，SITL/GS 有 BUILD_VERIFIED。**主动分配和主动恢复保持 IMPLEMENTED_UNVERIFIED**；不能称 SITL_VERIFIED、HITL_VERIFIED 或 FLIGHT_VERIFIED。

按用户要求在此停下，没有为了扩大完成度继续启动新一轮测试。源代码与产物归档在仓库外，未部署。

## S. Remaining risk

后续统一验证应覆盖真实基线/延迟/λ 精度与误报、模型误差及 fault 分类混淆、动态矩阵归一化和切换瞬态、控制器积分与双主动路径交互、恢复各阶段/重入/下降安全性、多机型和不支持模式退回原生、H743 最坏 CPU/栈/内存、ULog/MAVLink 带宽和 GS 端到端显示。新版本 FMUv6C 完整构建也留到下一阶段。

固定气动模型、噪声参数、质量推力尺度及控制参数尚未实测标定。无在线惯量/CG，方向权限不等于完整联合可达集合。任何 ACTIVE 实际能力都不能由这些离线测试推定。

## 文件与改动位置

| 路径组 | 改动部分与影响 |
| --- | --- |
| `src/modules/motor_health_monitor/` | RLS 状态、sigma/age、命令对齐、饱和门、质量 observer、诊断与 CLI；解决 excitation/confidence 耦合 |
| `src/modules/control_allocator/` | 动态矩阵 policy 与最小核心 hook；默认关闭，超时/结构变化回退 |
| `src/modules/ftc_recovery/` | 状态控制器、唯一仲裁器、输入适配和离线测试；实现实际控制入口与重入 |
| `src/modules/mc_rate_control/MulticopterRateControl.cpp/.hpp` | 只对送入原生控制器的局部 rate/thrust 进行仲裁，核心提交独立 |
| `src/modules/ftc_control_monitor/`、`ftc_supervisor/` | 方向权限与实际状态聚合，消除 invalid=normal |
| `msg/`、`src/modules/logger/logged_topics.cpp` | 新状态合同与日志订阅 |
| `src/modules/mavlink/`、`Tools/merivus/verify_ftc_telemetry.py` | v2 扩展字段与发送/验证合同 |
| GroundStation `src/Vehicle/VehicleFtcStatusFactGroup.cc/.h` | 解码、valid/stale 区分、中文文本和 motor roles |
| GroundStation `custom/res/Merivus/FtcStatusPanel.qml`、`CommandCenterOverlay.qml` | 详情与 Tooltip，主 H/E 保持简洁 |
| GroundStation `schemas/mavlink/`、`libs/mavlink/include/mavlink/v2.0/merivus*`、`tools/dev/test-ftc-telemetry-contract.ps1` | 两仓 XML 和生成头同步、消费契约检查 |
| 两仓 `docs/` 与 Firmware `AGENTS.md` | 当前契约统一，旧测试资料标明历史范围，阶段证据单独记录 |

本次新固件代码文件清单（相对 Firmware 仓库，排除先保护的旧改动）：

- `Tools/merivus/verify_ftc_telemetry.py`
- `msg/CMakeLists.txt`
- `msg/FtcAllocationStatus.msg`
- `msg/FtcArbitrationStatus.msg`
- `msg/FtcControlAuthority.msg`
- `msg/FtcModelStatus.msg`
- `msg/FtcRecoveryStatus.msg`
- `msg/FtcSystemStatus.msg`
- `msg/MotorHealthStatus.msg`
- `src/modules/control_allocator/ControlAllocator.cpp`
- `src/modules/control_allocator/ControlAllocator.hpp`
- `src/modules/control_allocator/FtcAllocationPolicy.hpp`
- `src/modules/ftc_control_monitor/FtcControlMonitor.cpp`
- `src/modules/ftc_control_monitor/FtcControlMonitor.hpp`
- `src/modules/ftc_recovery/CMakeLists.txt`
- `src/modules/ftc_recovery/FtcControlContractTest.cpp`
- `src/modules/ftc_recovery/FtcRateInput.hpp`
- `src/modules/ftc_recovery/FtcRecovery.cpp`
- `src/modules/ftc_recovery/FtcRecovery.hpp`
- `src/modules/ftc_recovery/FtcRecoveryArbiter.hpp`
- `src/modules/ftc_recovery/FtcRecoveryController.hpp`
- `src/modules/ftc_supervisor/FtcSupervisor.cpp`
- `src/modules/ftc_supervisor/FtcSupervisor.hpp`
- `src/modules/logger/logged_topics.cpp`
- `src/modules/mavlink/message_definitions/v1.0/merivus_ftc.xml`
- `src/modules/mavlink/streams/MERIVUS_FTC_CONTROL_STATUS.hpp`
- `src/modules/mavlink/streams/MERIVUS_FTC_DIAGNOSTICS.hpp`
- `src/modules/mavlink/streams/MERIVUS_FTC_MOTOR_STATUS.hpp`
- `src/modules/mavlink/streams/MerivusFtcTelemetry.hpp`
- `src/modules/mc_rate_control/MulticopterRateControl.cpp`
- `src/modules/mc_rate_control/MulticopterRateControl.hpp`
- `src/modules/motor_health_monitor/CommandAlignment.hpp`
- `src/modules/motor_health_monitor/EffectivenessEstimator.hpp`
- `src/modules/motor_health_monitor/MotorEffectivenessEstimator.cpp`
- `src/modules/motor_health_monitor/MotorEffectivenessEstimator.hpp`
- `src/modules/motor_health_monitor/MotorEffectivenessEstimatorTest.cpp`
- `src/modules/motor_health_monitor/MotorHealthMonitor.cpp`
- `src/modules/motor_health_monitor/MotorHealthMonitor.hpp`
- `src/modules/motor_health_monitor/RigidBodyObserver.hpp`
- `src/modules/motor_health_monitor/motor_health_monitor_params.c`

## 产物标识

- 固件源码归档 SHA-256：`812878d82382a3526491701a2986574d7076535df0afdf284c491adca2e33b2b`。
- 地面站源码归档 SHA-256：`27edc4d35f201ca7b585d23890d33201825258cdafdb2f764edbbfeb5c9b6acd`。
- SITL `bin/px4` SHA-256：`cb8883311a2a8dc7d3d704b91d2feb03080fbd0209d43d01e3a7c5fe4db9e351`。
- GS `staging/MERIVUS.exe` SHA-256：`0aa3590fe79f49f3eead63323f9e012231974c1d1a04c69844a4771b2bc1b01e`。

源码归档是上述代码提交的根仓 git archive，子模块仍由对应 gitlink 锁定，并非包含全部依赖的离线安装包。归档、构建日志、测试输出及 manifest 位于 `E:/MERIVUS-ftc-backup-20260908`。
