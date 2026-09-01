# FTC 参数

PX4 参数名最多 16 个字符，因此需求中的 `FTC_CA_ENABLE`、`FTC_REC_ENABLE` 分别实现为 `FTC_CA_EN`、`FTC_REC_EN`。所有危险入口默认 0。

| 类别 | 参数 | 默认值 | 当前作用 |
|---|---|---:|---|
| MON | `FTC_MON_EN` | 0 | 启动整个 FTC 观察层 |
| MON | `FTC_MIN_THR` | 0.15 | 估计最小平均电机指令 |
| EST | `FTC_LPF_TC` | 0.20 | 输入低通时间常数 |
| EST | `FTC_EXC_MIN` | 0.025 | 最小激励 |
| EST | `FTC_BASE_T` | 5.0 | 健康基线学习时间 |
| EST | `FTC_EST_RATE` | 0.30 | lambda 最大变化率 |
| EST | `FTC_EST_FORG` | 0.995 | RLS 遗忘因子 |
| EST | `FTC_EST_LMIN` | 0.10 | lambda 下界 |
| EST | `FTC_EST_IXX/IYY/IZZ` | 0.02/0.02/0.04 | 标称惯量近似 |
| FAULT | `FTC_RES_THR` | 1.0 | 模型残差有效门限 |
| FAULT | `FTC_HLTH_MIN` | 0.70 | 退化门限 |
| FAULT | `FTC_FAIL_MIN` | 0.25 | 失效门限 |
| FAULT | `FTC_CONF_MIN` | 0.60 | 最小估计置信度 |
| FAULT | `FTC_FAIL_T` | 1.0 | 故障持续时间 |
| FAULT | `FTC_FAULT_P` | 0.65 | 分类概率门限 |
| FAULT | `FTC_FAULT_VIB` | 8.0 | 振动归一化门限 |
| FAULT | `FTC_FAULT_EXT` | 0.70 | 外部扰动门限 |
| CA | `FTC_CA_SHADOW` | 0 | 矩阵导出与 shadow 分配 |
| CA | `FTC_CA_EN` | 0 | 保留接管参数，当前无执行路径 |
| CA | `FTC_CA_ATT_MIN` | 0.35 | roll/pitch 权限门限 |
| CA | `FTC_CA_YAW_MIN` | 0.20 | yaw 权限门限 |
| CA | `FTC_CA_THR_MIN` | 0.25 | thrust 权限门限 |
| IMPACT | `FTC_IMPACT_EN` | 0 | 冲击/硬着陆检测 |
| IMPACT | `FTC_IMPACT_ACC` | 30.0 | 加速度门限 |
| IMPACT | `FTC_IMPACT_JRK` | 120.0 | jerk 门限 |
| LOC | `FTC_LOC_EN` | 0 | 失控检测 |
| LOC | `FTC_LOC_THR` | 0.70 | LOC score 门限 |
| REC | `FTC_REC_EN` | 0 | 恢复状态机和候选生成 |
| REC | `FTC_REC_ACT` | 0 | 保留仲裁参数，当前故意无效 |
| REC | `FTC_REC_RATE` | 2.0 | 候选最大角速度 |
| REC | `FTC_REC_KD` | 0.8 | 角速度阻尼增益 |
| REC | `FTC_REC_ALT` | 3.0 | 恢复最低高度 |
| SIM | `FTC_SIM_EN` | 0 | SITL 故障注入总开关 |
| SIM | `FTC_SIM_MOT/EFF/RAMP/INT` | 1/1.0/0/0 | 电机、目标效能、渐变和间歇设置 |

参数生成检查当前识别 39 个 `FTC_` 参数。`FTC_MON_EN`、`FTC_CA_EN`、`FTC_REC_EN`、`FTC_REC_ACT`、`FTC_SIM_EN` 均默认 0。
