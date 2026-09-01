# MERIVUS 测试矩阵

## 验证等级

| 等级 | 含义 | 典型证据 |
| --- | --- | --- |
| L0 Static | 路径、接口、格式、参数/uORB 生成输入和文档一致性 | `rg`、Git diff、GitNexus、格式/元数据检查 |
| L1 Host | 不依赖 PX4 固件的主机可执行检查 | 纯算法测试、脚本检查 |
| L2 Build | 目标固件或 SITL 完整构建 | 构建日志、目标、工具链、SHA |
| L3 Unit | 模块/库级自动测试 | 测试用例、输入输出和边界断言 |
| L4 SITL | 仿真闭环和故障/协议场景 | SITL 日志、状态转移、重复步骤 |
| L5 ULog Replay | 日志时间对齐、阈值与误报/漏报分析 | ULog、分析脚本、基线窗口 |
| L6 HITL | 飞控硬件参与的闭环仿真 | 硬件、固件 SHA、场景与结果 |
| L7 Bench | 拆桨/受控台架的传感器、通信和输出检查 | 设备清单、参数、日志、急停条件 |
| L8 Tethered Flight | 系留或等效受限飞行 | 风险评审、现场记录、退出条件 |
| L9 Flight Validation | 批准配置的完整飞行验证 | 发布身份、环境、逐项验收与复盘 |

达到某一级不自动包含所有更低级证据；每次记录应明确实际执行项。

## 功能最低矩阵

| 功能 | 开发合入最低目标 | 发布/实机最低目标 | 当前已知状态 |
| --- | --- | --- | --- |
| 纯文档/索引 | L0 | L0 | 本轮执行 L0，未构建 |
| FMUv6C BSP / V6C22 传感器 | L0 + L2 | L7；飞行发布需 L9 | `IMPLEMENTED_UNVERIFIED` |
| Hyper982 GNSS / yaw | L0 + L2 + L4 | L7 + L9 | 配置合同存在，未在本轮复验 |
| HyperLte / MAVLink 带宽 | L0 + Unit + L4 | L7 + L9 | 代码/合同存在，未在本轮复验 |
| Swarm 两端协议 | L3/Mock + L4 | L7 → L8 → L9，按 1/2/6 机升级 | `IMPLEMENTED_UNVERIFIED` |
| FTC RLS estimator | L1 + L3 + L4 + L5 | 只观察时 L7；用于决策前 L9 数据集 | 核心 `HOST_VERIFIED`，集成未验证 |
| FTC fault classification | L3 + L4 + L5 | L7/L9 标定 | `IMPLEMENTED_UNVERIFIED` |
| FTC matrix shadow / authority | L3 + L4 + L5 | L6；只观察台架 L7 | `IMPLEMENTED_UNVERIFIED` |
| FTC impact / LOC | L3 + L4 + L5 | L7/L8/L9 分场景标定 | `IMPLEMENTED_UNVERIFIED` |
| FTC recovery candidate | L3 + L4 + L5 | L6 + L8；主动控制需 L9 | `IMPLEMENTED_UNVERIFIED`，未接管 |
| FTC active allocation/recovery | L3 + L4 + L5 + L6 | L7 + L8 + L9 | `SKELETON` / 未实现，禁止实机启用 |
| SITL FTC injection | L3 + L4 | 不适用实机 | `IMPLEMENTED_UNVERIFIED` |
| ULog topic contract | L2 + L4 + L5 | 相关功能发布前完成 | 源码订阅存在，尚无实际 ULog |

## 本轮验证边界

本轮没有修改程序行为，也未执行 PX4 完整构建、SITL、HITL 或实机测试。验证仅覆盖 GitNexus 索引、Git/路径审计、源码—文档静态一致性、Markdown 链接/格式与参数/uORB 声明检查。后续不得把本次文档提交当作 L2 或更高等级证据。
