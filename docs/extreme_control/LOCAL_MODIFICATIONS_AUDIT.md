# 本地修改审计

## 结论与证据边界

产品 Git 历史从提交 `b491015d6f568067894f0a688142b30c2d3f33af` 重新开始，初始提交一次性导入完整源码，因此无法仅凭本仓库证明其对应的官方 PX4 精确 SHA。`upstream/release/1.14` 可用，但与产品历史无共同祖先。

本机另有官方 PX4 v1.14.4 合同源码目录。对初始导入提交的 6001 个 blob 做内容哈希比较后：5725 个与 v1.14.4 同路径文件逐字节一致，116 个同路径文件不同，160 个仅存在于导入快照，v1.14.4 有 9 个额外文件。这证明代码来自 PX4 v1.14 系列，但不能把初始导入等同于 v1.14.4。

## A. 可证明的 MERIVUS 修改

从初始导入到冻结 HEAD 共变更 50 个路径，净计约 961 行新增、637 行删除，主要包括：

- FMUv6C/V6C22：增加 BMI088 驱动与 V6C02/V6C22 硬件识别、SPI 映射和传感器启动分流。
- 板级默认参数：Hyper982 GPS1、HyperLte/TELEM1、GPS yaw、高度参考、MAVLink 串口带宽配置。
- MAVLink：串口数据率物理上限、Magic/Custom 模式语义、数据率单测、副 GNSS 精度字段。
- Swarm：v1.14 待机状态适配、MAV_CMD 常量解耦和格式修复。
- CI/构建：旧 PX4 工具链、macOS、Actions、SITL 与 FMUv6C 流程兼容。
- 文档：MERIVUS 构建刷写、V6C22、RTK/4G 和 CI 契约。

## B. 未被产品提交改动的 PX4 核心

从初始导入到冻结 HEAD，以下核心路径没有产品层修改：

- `src/modules/ekf2`
- `src/modules/mc_att_control`
- `src/modules/mc_rate_control`
- `src/modules/mc_pos_control`
- `src/modules/commander`
- `src/modules/flight_mode_manager`
- `src/modules/control_allocator`
- `src/modules/logger`

本项目继续把这些模块视为成熟核心和安全边界，不在观察阶段修改其控制输出逻辑。

## C. 来源无法精确确认的代码

初始导入本身包含 swarm_node、额外板卡、消息和启动项，也与本机 v1.14.4 合同源码存在版本差异。由于上游提交 SHA 在导入时没有保留，这些差异只能标记为“导入前已存在/来源未完全可证”，不能全部归因于当前用户或官方 v1.14.0。

## D. FMUv6C / Pixhawk 6C 定制影响

必须保留的产品差异：

- `boards/px4/fmu-v6c/default.px4board` 的 BMI088 与 swarm_node 配置。
- `boards/px4/fmu-v6c/init/rc.board_sensors` 的 V6C02/V6C22 BMI088 分流。
- `boards/px4/fmu-v6c/src/board_config.h`、`manifest.c`、`spi.cpp` 的硬件版本和 SPI 契约。
- `boards/px4/fmu-v6c/init/rc.board_defaults` 的 GNSS/4G/MAVLink 产品默认值。

FTC 代码只能追加独立配置，不得重写上述硬件识别或通信默认值。

