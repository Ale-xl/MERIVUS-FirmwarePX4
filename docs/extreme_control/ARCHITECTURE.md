# 架构

## 数据流

```text
Sensors -> EKF2 -> Existing PX4 controllers -> torque/thrust setpoint
                                      |                 |
                                      |                 v
                                      |          Control Allocator -> actuator_motors -> outputs
                                      |                                  |
                                      +----------------------------------+
                                                                         v
                     Motor Health Monitor <- angular velocity/acceleration/status
                               |
                               +-> continuous effectiveness + confidence + residual
                               |
                               +-> adaptive allocation shadow (log only)
                               |
                               +-> impact/recovery observations (future, no command output)
```

## 长期不变量

1. `FTC_MON_EN=0` 时不运行估计，不改变正常飞控行为。
2. 第一阶段模块只订阅现有 uORB 数据并发布 FTC 诊断 topic。
3. shadow mode 不发布执行器或控制 setpoint。
4. 低激励、未解锁、已落地、数据超时或数值异常时模型必须无效，不能把噪声解释为电机损伤。
5. intervention 默认关闭，且在本阶段没有执行路径。
6. EKF2、mc_att_control、mc_rate_control、mc_pos_control 与 Commander 的控制逻辑保持不变。

## 版本适配

本机 v1.14 已提供 `actuator_motors`、`vehicle_angular_velocity.xyz_derivative`、`vehicle_acceleration`、`control_allocator_status`、rotor effectiveness matrix、pseudo-inverse 和 sequential desaturation。FTC 使用这些真实接口，不复制 PX4 main 的新版本实现。
