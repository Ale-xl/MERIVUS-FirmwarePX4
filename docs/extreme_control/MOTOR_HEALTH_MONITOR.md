# Motor Health Monitor

目标模块为 `motor_health_monitor`。首版估计器使用 `actuator_motors`、`vehicle_angular_velocity`、`vehicle_status`、`vehicle_land_detected` 和可选 `esc_status`，发布连续健康度、效能、残差、置信度和故障掩码。`vehicle_acceleration` 已纳入冲击检测设计，但在首版电机效能估计中不作为判据，避免把风扰或平动响应直接归因到单个电机。

第一阶段约束：模块只观察、计算、发布和记录；不写执行器，不改 PID，不切模式。CLI 提供 `start`、`stop`、`status`。参数关闭、未解锁/落地、输入陈旧或激励不足时发布无效状态并重置故障持久计时。

算法初版采用轻量低通、角加速度响应残差、滑动激励统计与持久判定。个体电机辨识受输入相关性和未建模气动力影响，结果必须结合 confidence 与 `model_valid` 使用。

## 参数

- `FTC_MON_EN=0`：启动门，默认不运行。
- `FTC_MIN_THR=0.15`：允许估计的最小平均 motor command。
- `FTC_LPF_TC=0.20 s`：命令和角加速度低通时间常数。
- `FTC_EXC_MIN=0.025`、`FTC_BASE_T=5 s`：激励和健康基线门。
- `FTC_EST_RATE=0.30 /s`：效能变化率上限。
- `FTC_RES_THR=1.0`、`FTC_CONF_MIN=0.60`：模型有效门。
- `FTC_HLTH_MIN=0.70`、`FTC_FAIL_MIN=0.25`、`FTC_FAIL_T=1 s`：持久分类门限。
- `FTC_CA_SHADOW=0`：只读候选补偿输出，默认关闭。

模块在 SITL 和 FMUv6C board config 中编译；`rc.mc_apps` 仅在 `FTC_MON_EN=1` 时启动。MSVC `/W4 /WX` 编译的纯估计器对象实测为 1136 bytes；完整 NuttX heap、共享 work queue stack、Flash 和 CPU 必须以目标构建及 `perf`/`top` 为准。
