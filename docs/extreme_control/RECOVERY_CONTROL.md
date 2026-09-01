# 恢复控制

状态机与候选控制律：`IMPLEMENTED_UNVERIFIED`。真实控制仲裁：`SKELETON`。

触发源为 impact、LOC、confirmed motor fault 和 authority degradation。资格门检查 armed、非 landed、旋翼机、状态新鲜、最低高度和是否仍具可控性。

RATE_DAMPING 生成 `omega_sp = constrain(-Kd * omega)`；随后生成保持当前 yaw 的水平四元数、body-Z 推力矢量和 reduced-yaw 请求，再经过姿态、高度与 control reentry 阶段。EMERGENCY_LAND 只生成受限下降候选。

所有候选仅发布到 `ftc_recovery_status`。当前没有向 PX4 正常 setpoint 管道的写入者，`FTC_REC_ACT` 置 1 也只在 status 中显示 requested/inactive。未来连接必须提供单一 setpoint owner、无竞争仲裁、Commander/failsafe 协调、超时 fallback 和完整安全门验证。
