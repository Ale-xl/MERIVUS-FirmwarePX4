# FTC 恢复候选与仲裁

`ftc_extreme_state_monitor` 使用姿态/角速度跟踪误差、饱和、控制裕度、推进退化、加速度、jerk 和垂直运动判断 Impact、硬着陆与 LOC。`ftc_recovery` 在满足资格门时生成角速度抑制、推力方向、姿态恢复、垂直速度/高度稳定、重新交接或紧急下降候选。

恢复候选只通过 `FtcRateInput` 与正常 rate/thrust setpoint 仲裁，随后仍进入原生 `mc_rate_control` 和唯一 ControlAllocator。候选包含有限值、姿态、垂直状态、authority、时效和状态机门；正常输入出现非有限值时，FTC 混合权重立即释放，不制造替代正常指令。

7 次正常飞行没有 Impact 或 `LOC_LOSS_OF_CONTROL` 误报；一次会话短暂出现 `LOC_DISTURBED`。候选状态机和退出门有 Host 证据，但没有真实扰动下的闭环候选轨迹。Active Recovery 从未开启，状态为 `IMPLEMENTED_UNVERIFIED`，默认 `FTC_REC_ACT=0`。
