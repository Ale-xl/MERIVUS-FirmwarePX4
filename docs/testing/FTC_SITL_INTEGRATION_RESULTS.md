> 历史测试资料：适用于文中记录的旧提交；当前软件契约及验证边界见 [阶段总结](FTC_SOFTWARE_STAGE_REPORT.md)。

# FTC SITL 阶段集成验证结果

日期：2026-09-04  
结论：**BLOCKED**

## A. 环境与工作区

- Windows 源目录：`E:\MERIVUS\FirmwarePX4`、`E:\MERIVUS\GroundStation`。
- Ubuntu 工作目录：`/home/cwkj/MERIVUS/FirmwarePX4`、`/home/cwkj/MERIVUS/GroundStation`。
- SSH 入口：`px4vm`，Ubuntu 20.04.4，2 核、7.7 GiB 内存。
- 工具链：GCC/G++ 9.4、CMake 3.16.3、Ninja 1.10、Python 3.8.10、ARM GCC 9.2.1、Gazebo Classic 11.12。
- Firmware 分支/基线：`research/extreme-control-v1` / `e2d59728b93b5bf3aa300ac175373fa75c255f58`。
- GroundStation 分支/基线：`research/groundstation-ftc-ui` / `1d49372366609afb91e19f42946637bfaf341e8f`。
- 迁移后两侧 Git 对象、工作树状态、文件数量和 CRLF 分布已核对；未执行 `git reset`、`git clean`、`git restore` 或全仓库 `dos2unix`。

## B. 固件构建

| 目标 | 结果 | 产物 SHA-256 |
| --- | --- | --- |
| `px4_sitl_default` | PASS | `cb0c4f6bf44a60a12d504f54d11681a1ce3dc89aebaa7f3d679830d9afb44f6e` |
| `px4_fmu-v6c_default.elf` | PASS | `b8d9b4483ac302827c88524b852164bcd7f5a48cc284672a462bbe8acac35809` |
| `px4_fmu-v6c_default.bin` | PASS | `e921a276ce749041ba69edcc0e886472367d0eaad325631635e8b64598617e15` |
| `px4_fmu-v6c_default.px4` | PASS | `190ada6fd21c464592c27b8bced5597d8780db85ff2ca5f7c6d59ac05ac1f204` |

FMUv6C 固件占用 1,946,440 / 1,966,080 B（99.00%），余量 19,640 B。产品链路使用 MAVLink，不使用 ROS 2/DDS，因此板级默认关闭未使用的 uXRCE-DDS client 后通过容量检查；FTC、Swarm 和 BMI088 能力保留。

## C. FTC 模块与安全边界

- `motor_health_monitor`、`ftc_control_monitor`、`ftc_extreme_state_monitor`、`ftc_recovery`、`ftc_supervisor` 均已在 SITL 启动并运行。
- FTC 观察、影子分配、极端状态、恢复候选和系统状态 uORB topic 均有输出。
- 全程保持 `FTC_CA_EN=0`、`FTC_REC_ACT=0`，未建立 ACTIVE 执行器接管路径。
- 测试收口状态：`FTC_SIM_EN=0`、`FTC_SIM_EFF=1.0`。

## D. 遥测契约

- 两仓核心 XML SHA-256 一致：`0ae936460c2489f33cebaaba018b2532dd258486d50d1fd835dbc91fc6fc064d`。
- `Tools/merivus/verify_ftc_telemetry.py`：PASS。
- 12 秒原始 UDP 帧采样：60000=51、60001=51、60002=102、60003=10；与 5/5/10/1 Hz 配置一致。
- PX4 stream 状态中的消息尺寸分别为 90/36/50/77 B。

## E. GroundStation

- Qt 5.15.2 / MSVC 2022 Release 重新配置、编译、链接：PASS。
- `MERIVUS.exe` SHA-256：`99e084ef11db0eae71720206407ef7a793761b725e44aa88ddfedf305da46a18`。
- 首次 staging 启动返回 `0xC0000135`；构建脚本执行过 `windeployqt`，但 staging 中没有 Qt5 DLL。显式执行 Qt 官方 `windeployqt --release --qmldir ... MERIVUS.exe` 后启动成功。
- `MERIVUS.exe` 运行并响应，监听 `0.0.0.0:14550`；PX4 对 Windows `192.168.135.1` 的专用 MAVLink 链路识别到有效 GCS heartbeat，接收丢包率 0%。
- 暂停四条 FTC 流 5 秒期间，标准心跳持续有效且 GroundStation 仍响应；这覆盖了 3 秒 FTC 过期条件。当前自动化环境无法读取 Windows 原生应用界面，因此 `N/A`/“FTC 数据已过期”的视觉文案仍需一次人工确认。

## F. SITL 测试矩阵

| 工况 | 结果 | 关键观察 | ULog |
| --- | --- | --- | --- |
| NORMAL | PASS | 起飞、悬停、降落；无故障、LOC 或撞击 | `NORMAL.ulg` |
| `FTC_SIM_EFF=1.0` | PASS | 注入状态有效；模型和控制裕度正常；ACTIVE 关闭 | `EFF_100.ulg` |
| `FTC_SIM_EFF=0.70` | BLOCKED | 注入前模型有效；约 5 秒内下降/触地，模型失效，未得到可信 0.70 效能趋势 | `EFF_70_FINAL_BLOCKED.ulg` |
| `FTC_SIM_EFF=0.40` | BLOCKED | 1 秒内四电机被归类为外部扰动，模型失效，饱和掩码 15，检测到撞击/LOC | `EFF_40_FINAL_BLOCKED.ulg` |

0.70/0.40 失败不是电机序号错配：Gazebo `input_index 0..3` 与 PX4 四旋翼几何中的前右、后左、前左、后右顺序一致。当前核心阻塞是故障注入后的估计/判别链无法在飞行器失控前给出可信的目标电机效能趋势。

## G. ULog 证据

证据目录：`/home/cwkj/MERIVUS/test-artifacts/20260903-013501`

| 文件 | 字节 | SHA-256 |
| --- | ---: | --- |
| `NORMAL.ulg` | 77,692,369 | `ebf6700ebd1721934138426a7be0203b0aed213abced89420215fe2a5fb7d059` |
| `EFF_100.ulg` | 18,670,520 | `9f0198d4d44020c2c3f6a9b0eec163c51b19230b514db23363c6492453e20d86` |
| `EFF_70_FINAL_BLOCKED.ulg` | 45,592,301 | `bc2af0fc7c0b2b2b614f88813ddbc0f606ee68c771aa2482200b8ef1ec4096fa` |
| `EFF_40_FINAL_BLOCKED.ulg` | 25,725,907 | `edba1cfd704b6a510912d325830be83c224b5a756dec8d1a6d18a3af11027534` |

这些二进制证据不提交 Git。

2026-09-07 补充审计发现：上述四份 ULog 只记录了提前发布的 `ftc_simulation_status` 和
`ftc_effectiveness_matrix`，未记录其他 FTC 诊断 topic。因此本节只能证明文件身份，不能支持效能、
置信度、模型有效性、authority、LOC 或 recovery 的精确时间对齐。原因和修复见
`FTC_EFFECTIVENESS_FAILURE_ANALYSIS.md`；原人工 listener 结果仍保留为人工观察，不再表述为 ULog 证据。

## H. 本轮源码变更

- 版本生成链统一只匹配正式 `vX.Y.Z` 标签，避免 `archive/...` 标签破坏版本解析。
- 修正 control allocator 中 FTC 归一化参数的声明位置和初始化。
- 补齐 FTC control monitor 对 control allocator 头文件的包含路径。
- 修正 MAVLink FTC stream 的枚举转换和 NuttX 浮点舍入兼容。
- 修正 motor effectiveness 输出结构的初始化方式。
- FMUv6C 默认关闭未使用的 uXRCE-DDS client，以满足固件容量契约。

## I. 未解决问题与下一步

1. **核心阻塞**：定位 0.70/0.40 注入后为何整体被识别为外部扰动、模型迅速 invalid，并建立不会依赖瞬时激励丢失的可重复判别契约。
2. `ftc_recovery` 可在启动/瞬态后进入 `ABORTED`，且当前状态机不会自动回到 `MONITORING`；应单独做状态机影响分析和回归，不在本轮临时打补丁。
3. GroundStation 构建脚本的 Qt runtime 自动部署需要修复和复测；当前 staging 已通过手工执行官方部署工具恢复。
4. 人工确认 GroundStation 在 FTC 断流 3 秒后显示 `N/A`/“FTC 数据已过期”，恢复数据后界面恢复。

在第 1 项解决并重新通过 0.70/0.40 阶段前，不进入 ACTIVE FTC 控制、真实硬件或刷写验证。
