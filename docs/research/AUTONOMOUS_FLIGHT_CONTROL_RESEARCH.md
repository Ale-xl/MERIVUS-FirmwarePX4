# MERIVUS 自主飞控算法研究系统：第一阶段

状态：`CANDIDATE_005_SCREENING_PENDING`。候选图生成、PX4 SITL 控制入口、仿真试验与 ULog 判定代码已在隔离的 Ubuntu 虚拟机完成构建和闭环对照。[首轮实验记录](experiments/round-001/README.md)与[候选 002 发现集](experiments/round-002/RESULT.md)均未通过正常悬停回退门槛。[第二轮协议](experiments/round-002/PROTOCOL.md)固定了随机数种子、阵风事件和物理位移评分；[第三轮预注册协议](experiments/round-003/PROTOCOL.md)针对自然波动设置绝对容差和交替试验顺序。目标是正常执行器条件下的多旋翼定点悬停。此前针对定位源、动力链与原生参数的悬停审计仍是实机问题诊断依据；SITL 结果不能替代这些检查。

## 四个角色与边界

| 角色 | 输入与输出 | 当前实现 |
| --- | --- | --- |
| Researcher | 一手论文及 PX4 官方资料；每条结论附来源、适用工况、控制层级与可证伪假设 | `research.py` 自动抓取 11 个主题的 Crossref 元数据；全文归纳与证据审核尚未自动化 |
| Algorithm Designer | 从有量纲的算子组合候选图，限定状态量、幅值与计算复杂度 | `candidate.json`、`candidate.py`；当前候选只是有界残差补偿，不宣称新算法 |
| Engineer | 将生成候选接到 PX4 速度环输出与加速度到推力转换之间 | 仅 SITL 编译；默认关闭，`shadow` 只计算，`active` 只对悬停位置设定点施加 |
| Experimental Scientist | 构建、试验、ULog、硬门、同种子比较、保留或淘汰 | `cycle.py`、`run_sitl.py`、`evaluate.py`；已在 Ubuntu 20.04.4、Gazebo Classic 11.12.0 与 PX4 SITL 工具链实测 |

候选结构采用拓扑有序的类型图。输入只有速度误差（m/s）和加速度残差（m/s²）；候选输出为加速度修正（m/s²）。生成器检查输入单位、拓扑顺序、节点数量、增益、滤波时间常数和末端限幅。当前残差是“测得速度导数减上一周期加速度设定点”的代理量，包含执行器动态、控制延迟及估计噪声；不能将其直接称为真实外扰。候选 003 的水平修正每轴不超过 0.35 m/s²，Z 轴增益为零，全局修正上限为 0.5 m/s²。无效或不完整输入、异常周期或数值失效时退回 PX4 原输出。

Researcher 可用 `python3 Tools/merivus/flight_control_research/research.py --output /absolute/results/literature.json` 批量抓取 11 个主题的 DOI 元数据。输出统一标记 `METADATA_ONLY`；要形成数学假设，仍须读取原文并记录控制层级、状态量、带宽、验证平台和反例。首轮一手资料锚点如下：

| 来源 | 本项目可借鉴的范围 | 不可直接外推的结论 |
| --- | --- | --- |
| [PX4 v1.14 控制模块说明](https://docs.px4.io/v1.14/en/modules/modules_controller) | 位置 P、速度 PID 的原生接口和控制层级 | 更换一个控制律就能改善定位误差 |
| [TU Delft 的 INDI-SMC/SMDO 四旋翼研究](https://research.tudelft.nl/en/publications/quadrotor-fault-tolerant-incremental-sliding-mode-control-driven-/) | 增量控制和扰动观测器的耦合假设 | 其故障与风场实验结果可直接复制到本机悬停 |
| [IEEE 的 DOB-MPC 四旋翼研究](https://ieeexplore.ieee.org/document/10778610) | 观察器与预测控制的组合结构 | 论文轨迹跟踪性能等于 PX4 悬停收益 |
| [UZH/ETH 相关的 GP-MPC 残差动力学研究](https://arxiv.org/pdf/2102.05773v2) | 残差学习和标称模型分离的思路 | 高速轨迹结果代表低速悬停表现 |

PX4 原位置环、姿态环、角速度环、EKF2、allocator 与 failsafe 保持原有职责。研究模式只在 `CONFIG_ARCH_BOARD_PX4_SITL` 构建中存在，还要同时具有 `PX4_SIM_MODEL` 与 `MERIVUS_AFCR_MODE=shadow|active`。模式只在飞行中、非接地且 XYZ 位置设定点有效时进入；失效保护重算时关闭。真实固件无该入口。SITL 使用已有 `debug_vect` 以 `AFCR_DA` 记录候选加速度修正；设置 `SDLOG_PROFILE=163` 包含 debug profile。首轮日志中的 `AFCR_ACC` 是修正前后的总候选加速度，含义不同。没有增加参数或 uORB 类型。

## 每轮实验合同

1. AI 只修改 `Tools/merivus/flight_control_research/candidate.json`，由生成器生成 `ResearchCandidateSpec.hpp`。`cycle.py` 检查相对 HEAD 的更改路径；评价器、试验器与门限必须经单独审查。此路径检查是流程约束，不是恶意代码沙箱。
2. Linux 运行 `python3 Tools/merivus/flight_control_research/cycle.py --output /absolute/results/round-002 --dialect /absolute/generated/pymavlink/dialect.py --seeds 11 12`。输出目录须不存在；脚本运行单测，先执行 `make px4_sitl_default -j4` 生成 MERIVUS MAVLink 头文件，再执行 `make px4_sitl_default sitl_gazebo-classic -j4`。进入 Active 前先运行阵风 `shadow` 并检查 `AFCR_DA` 的有限值、样本数与限幅；每个种子随后运行原生 PX4 与候选 Active 的正常、固定风、阵风和较重载荷场景。MAVLink dialect 路径需提供适配本仓库的已生成 Python 文件。
3. 每次独立 Gazebo 会话保存场景、种子、源码提交、世界或模型、日志和 ULog；评分读取仿真真值 `vehicle_local_position_groundtruth`、控制器位置设定点、allocator 状态、电机输出和实际飞行模式。真值、设定点或 allocator 数据缺失时拒绝评分；窗口中必须保持已解锁的 Position/Loiter。当前载荷是起飞前固定 1.8 kg，而非空中突加载荷；第二轮增加仿真第 46–51 秒的独立阵风事件。
4. 原始真值相对设定点的 XY/Z 峰值与分配失败、飞行模式构成硬门；物理悬停位移以窗口前 2 秒真值为锚点评分。首轮和第二轮的失败判定分别保留在对应实验记录；当前周期执行[第四轮预注册协议](experiments/round-004/PROTOCOL.md)中的正常、固定受扰及阵风门槛。所有阈值均为 SITL 候选筛选条件，不能用作实机安全阈值。

## 下一阶段

- 为 Researcher 建立 IEEE、TU Delft、ETH、PX4 等一手来源的可追溯证据库；逐项登记 INDI、L1 adaptive、H∞、DOB/ESO、滑模、MPC、自适应、容错、在线辨识、控制分配、残差学习的控制层级、假设、采样率与失效模式。
- 用阵风开始/停止以及空中载荷变化建立可计时的事件，加入恢复时间、超调、饱和连续时长、估计器创新和重置、控制周期/CPU 门限。增加训练/保留测试场景分离，避免根据同一组种子反复挑选。
- 下一轮先固定评价合同和保留种子，再形成有控制层级、可观测量、带宽和失效假设的候选；用 `shadow` 检查输出分布及饱和，再用 `off`/`active` 完成同场景对照。首轮已证明流程能淘汰候选，但还不能证明该候选改善悬停。
- 实机阶段需另立机体、定位、台架与飞行批准合同。
