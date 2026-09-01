# 安全门

- G0 Baseline preserved：已完成。恢复 branch/tag 已验证。
- G1 Monitor compiles and logs：待完成 SITL 与 FMUv6C 构建、topic/CLI/ULog 验证。
- G2 SITL degradation identification：待完成连续注入与误报测试。
- G3 Online estimator validated：待完成 bounds、rate、excitation 和回放证据。
- G4 Allocator shadow validated：待完成候选输出与 nominal 对比。
- G5 Allocator intervention SITL：未开始。
- G6 Impact detector validated：未开始。
- G7 Recovery controller SITL：未开始。
- G8 HITL / hardware bench：未开始。
- G9 拆桨台架：未开始。
- G10 系留低风险飞行：未开始。

不得跳过前置 gate。首次刷入 FMUv6C 仍只允许 monitor/log/estimate；allocator intervention 与 recovery 必须关闭。

