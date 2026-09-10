# FTC 验证状态矩阵

2026-09-10 收尾。结果依据 [完整报告](FTC_FULL_VALIDATION_REPORT.md)，`HOST_VERIFIED` 只覆盖列出的性质，不能向真实闭环飞行外推。

| 功能 | 当前证据等级 | 本轮结果 / 未完成范围 |
| --- | --- | --- |
| Estimator | HOST_VERIFIED（限定理想模型） | 24 组静态 λ 真值、匹配延迟及低可观测门已量化；间歇跟踪失败；真实 SITL 稳定有效门失败。 |
| Baseline / 对齐 | IMPLEMENTED_UNVERIFIED（真实稳定估计） | 已建立真实飞行复现；7 次最终准入均失败。40 ms 不是已确认的最优实机延迟。 |
| Fault Detection / Isolation | IMPLEMENTED_UNVERIFIED | 正常飞行无退化/失效 mask 误报；尚无本轮物理故障注入的阳性定位证据。 |
| Fault Classification | HOST_VERIFIED（分类选择函数） | 4 项；移除仅凭 λ/全机振动的具体物理部件归因。不能据此宣称物理分类准确率。 |
| ESC 辅助诊断 | IMPLEMENTED_UNVERIFIED | SITL RPM 为合成信号，不是独立实测旋翼转速。 |
| Dynamic Matrix / Authority | BUILD_VERIFIED | 已复核源码与数据链；模型无效时 authority 无效。未完成有效故障 λ 下的方向余量与耦合能力验证。 |
| Shadow | IMPLEMENTED_UNVERIFIED | ULog 有候选及残差；没有有效模型条件下 candidate 优于 nominal 的闭环量化证据。 |
| Active Allocation | IMPLEMENTED_UNVERIFIED | 维持关闭；Host 验证持久性、软回退、上锁/着陆/不支持机型清零。 |
| Impact / LOC | SITL_VERIFIED（正常飞行检查） | 飞行窗口无 Impact、无 LOC_LOSS_OF_CONTROL 误报；一次短暂 LOC_DISTURBED。碰撞、重着陆、真正失控阳性测试未执行。 |
| Recovery Candidate | HOST_VERIFIED（控制器策略） | 状态序列、有界候选、非法输入拒绝与退出条件；真实触发场景未验证。 |
| Active Recovery | IMPLEMENTED_UNVERIFIED | 未开启。 |
| Vertical / Emergency Land / Re-entry | HOST_VERIFIED（控制器策略） | 真实恢复效果、下降轨迹与交接连续性尚未验证。 |
| Dual Active / 积分器 | IMPLEMENTED_UNVERIFIED | Supervisor 双 Active 状态有 Host 测试，双控制路径动态效果及积分器日志尚未验证。 |
| Supervisor | HOST_VERIFIED | 18 个状态/优先级测试；最新策略参与正常 SITL 回归。 |
| Logging | SITL_VERIFIED（本轮 7 会话） | 11 个 topic 均存在；保留 ULog、参数、事件和源提交。 |
| MAVLink v2 | SITL_VERIFIED（正常与停流） | 实际 5/5/10/1 Hz；实际后端解码 26092 包、0 CRC 错误；消息暂停与恢复已验证。未标定绝对链路丢包率。 |
| GroundStation backend | HOST_VERIFIED + SITL_VERIFIED（真实遥测） | Qt Test 8 个结果项全通过；实际 Windows↔VM 链路、停流及恢复通过。 |
| GroundStation GUI | BUILD_VERIFIED；连接可访问性已观察 | 实际窗口出现 UAV-1；截图/点击接口失败，FTC 详情视觉状态未完成自动确认。 |
| FMUv6C / SITL | BUILD_VERIFIED | 最终代码 `d4c3972f68` 均构建通过。 |
| Mass | IMPLEMENTED_UNVERIFIED | 默认无推力标定，未做 observe-only 标定实验。 |
| Inertia / CG | DESIGN / NOT_IMPLEMENTED（在线辨识） | 不报告有效在线辨识。 |
| HITL / BENCH / FLIGHT | 未验证 | 本轮没有真实硬件操作。 |

软件待办与硬件待办分开：估计器模型误差、间歇跟踪、有效性历史一致性、正向故障注入、Shadow 定量对照及全部 Active 场景仍属于软件/仿真待办，不交给用户当作硬件验收。
