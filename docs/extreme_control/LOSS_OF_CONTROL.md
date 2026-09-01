# 冲击与失控检测

状态：`IMPLEMENTED_UNVERIFIED`。

`ftc_extreme_state_monitor` 独立订阅 vehicle acceleration、angular velocity/acceleration、attitude、attitude/rate setpoint、local position、land state、actuator motors、allocator status、control authority 和 motor health。

冲击特征包括加速度模长、jerk、角加速度、姿态跳变和速度突变，输出 score、confidence、severity、axis、event timestamp，并通过 ground contact 与下降速度区分 hard landing。

失控检测融合姿态误差、角速度误差、分配失败、电机饱和、剩余权限、推进故障和下降速度，输出 NORMAL、DISTURBED、RECOVERY_RECOMMENDED、LOSS_OF_CONTROL、UNRECOVERABLE 及 reason mask。未解锁或已落地时不会升级 LOC 状态。

尚缺：时间窗口投票、传感器 clipping 独立证据、估计器 reset 排除、SITL/ULog ROC 标定。
