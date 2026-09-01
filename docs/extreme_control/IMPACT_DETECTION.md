# Impact Detection

状态：检测与状态发布为 `IMPLEMENTED_UNVERIFIED`；真实控制触发未连接。

检测不能只依赖姿态角阈值。候选证据包括角速度、角加速度、线加速度、jerk、控制饱和、动力健康和指令—响应不一致。正常特技动作、硬着陆、传感器毛刺和真实撞击必须在 SITL/日志回放中分别评估。

`ftc_extreme_state_monitor` 已融合这些特征，区分 external impact 与 hard landing，并与独立 LOC 状态机共享 `ftc_extreme_state`。检测事件只发布诊断；恢复模块读取该诊断后生成断开的候选，不切换模式、不写正常 setpoint。
