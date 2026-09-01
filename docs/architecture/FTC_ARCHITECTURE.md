# FTC / 极端环境增强架构

## 当前结论

FTC 已形成完整的诊断数据合同和断开的恢复候选链，但没有主动控制接管。现有 PX4 EKF2、姿态、角速度、位置、Commander 和 flight mode manager 保持原路径；唯一进入 `control_allocator` 的 FTC hook 是受 `FTC_CA_SHADOW` 控制的名义矩阵只读发布。

| 层级 | 已实现内容 | 当前行为 | 状态 |
| --- | --- | --- | --- |
| `OBSERVE` | 电机效能/故障、模型、冲击/LOC、顶层状态、ULog topic | 读取现有 uORB，发布诊断 | `IMPLEMENTED_UNVERIFIED`；纯估计器核心 `HOST_VERIFIED` |
| `SHADOW` | `B_nominal` 导出、`B_dynamic`、Sequential Desaturation 候选、权限 | 计算与真实 allocator 并行，不写执行器 | `IMPLEMENTED_UNVERIFIED` |
| `CANDIDATE` | rate damping、水平姿态、body thrust、reentry/emergency-land 候选 | 只写 `ftc_recovery_status` | `IMPLEMENTED_UNVERIFIED` |
| `ACTIVE CONTROL` | setpoint owner、allocator 切换、Commander/failsafe 仲裁 | 不存在；`FTC_CA_EN`、`FTC_REC_ACT` 无执行路径 | `SKELETON` / 未实现 |

## 模块与数据合同

```text
actuator_motors + angular velocity/acceleration + optional esc_status
          |
          v
 motor_health_monitor ---------------------> motor_health_status
          |                                  ftc_model_status
          |
          | lambda/confidence
          v
 ftc_effectiveness_matrix <--- control_allocator [read-only hook]
          |
          v
 ftc_control_monitor ----------------------> ftc_allocation_shadow
          |                                  ftc_control_authority
          v
 ftc_extreme_state_monitor ----------------> ftc_extreme_state
          |
          v
 ftc_recovery -----------------------------> ftc_recovery_status
          |
          v
 ftc_supervisor ---------------------------> ftc_system_status -> ULog

 simulator_mavlink --SITL injection-------> ftc_simulation_status -> ULog
```

## 模块职责

- `motor_health_monitor`：有界 RLS 效能估计、基线与激励门、故障概率/持续度/类别。无 ESC 遥测时进入 `imu_only` 并降低置信度。
- `ftc_control_monitor`：按 `B_dynamic(:,i)=B_nominal(:,i)*lambda_i` 构造动态矩阵，使用 PX4 Sequential Desaturation 计算 shadow 候选和剩余权限。
- `ftc_extreme_state_monitor`：融合加速度、jerk、角速度、角加速度、姿态/rate 误差、饱和、权限和电机健康，发布 impact、hard landing 与 LOC 状态。
- `ftc_recovery`：检查 armed、landed、机型、数据新鲜度、高度与可控性，只生成恢复候选。
- `ftc_supervisor`：聚合五类状态并发布唯一的 `ftc_system_status`；不拥有执行器、模式或 failsafe 权限。

## 有效与骨架模型

- 电机效能 RLS：核心主机测试已验证，PX4 模块集成尚未构建/SITL 验证。
- `ftc_model_status.mass`、`inertia`、`cg_offset` 已定义字段；质量和 CG 估计为 `SKELETON`，`mass_valid=false`、`cg_valid=false`。
- `FTC_EST_IXX/IYY/IZZ` 仅是刚体转动观测的标称惯量近似，在线惯量没有足够可观测性，`inertia_valid=false`。
- 故障具体类别和极端状态阈值未完成 SITL 混淆矩阵、ULog 标定与实机特征库验证。

## 安全不变量

1. 所有危险入口默认 0；`FTC_MON_EN=0` 时不运行估计。
2. `FTC_CA_SHADOW=0` 时 allocator 不发布 FTC 矩阵快照；即使启用也不改变原始分配输出。
3. FTC 模块不发布 `actuator_motors`、`actuator_outputs` 或正常 vehicle setpoint。
4. 未解锁、已落地、数据过期、激励不足或数值异常时模型不得成为有效故障证据。
5. SITL 注入只在 `FTC_SIM_EN=1` 时修改发往仿真器的控制通道，不是硬件路径。
6. 主动接管前必须建立唯一 setpoint owner、无竞争仲裁、Commander/failsafe 协调、超时 fallback，并达到测试矩阵规定的等级。

专项算法、参数、安全门和 ULog 合同继续保留在 `docs/extreme_control/`；本页是系统级入口，不替代那些验证文档。
