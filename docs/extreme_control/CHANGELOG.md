# Changelog

## 2026-08-31

- 审计仓库、产品历史、官方来源边界、子模块、FMUv6C 配置和构建目标。
- 创建 `backup/px4-v1.14-current-20260831` 与 `archive/px4-v1.14-current-20260831`。
- 从冻结 HEAD 创建 `research/extreme-control-v1`。
- 建立 FTC 架构、日志、测试和安全门文档。

## 2026-09-01

- 新增 `motor_health_monitor`，包含 CLI、参数启动门、低通/激励门、共享协方差 RLS、连续 effectiveness、confidence、residual 和持久故障 mask。
- 新增 `motor_health_status`、`ftc_allocation_shadow`、`ftc_simulation_status`，并接入默认 ULog topic。
- 在 SITL/FMUV6C board config 中编入监测器；默认 `FTC_MON_EN=0`，不改变启动与控制链路。
- 新增只读 allocation candidate shadow；`FTC_CA_SHADOW=0`，没有 intervention 实现。
- 在 POSIX `simulator_mavlink` 边界新增突发、渐变和间歇 motor effectiveness 注入；`FTC_SIM_EN=0`，硬件构建无执行路径。
- uORB/参数生成通过；纯估计器 MSVC `/W4 /WX` 与合成 50% 退化冒烟测试通过。
- 当前主机缺少 Ubuntu/ARM 工具链，SITL 与 FMUv6C 构建及运行测试待完成。
- 建立统一 `ftc_model_status`、`ftc_control_authority`、`ftc_extreme_state`、`ftc_recovery_status`、`ftc_system_status` 状态合同。
- 将电机健康升级为保守的多特征故障检测、隔离与分类；ESC 不可用时自动降级到 IMU/控制量观测。
- 抽象 effectiveness estimator，加入可配置遗忘因子、效能下界、激励度和转动刚体模型残差。
- `control_allocator` 新增默认关闭的只读矩阵导出；shadow 使用原生 sequential desaturation 计算动态矩阵候选和六轴残差。
- 新增控制权限、冲击/硬着陆、失控、恢复候选和 FTC Supervisor 独立模块。
- 所有接管参数默认 0，恢复候选与 PX4 正常 setpoint/actuator 链路物理断开。
