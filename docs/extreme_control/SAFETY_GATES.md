# 安全门

- G0 Baseline preserved：已完成。恢复 branch/tag 已验证。
- G1 Monitor compiles and logs：部分完成。uORB/参数生成通过，纯估计器 MSVC `/W4 /WX` 编译与合成响应测试通过；SITL 与 FMUv6C 构建、topic/CLI/ULog 运行验证未完成。
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

当前 Windows 工作机没有 WSL 发行版、GNU make、CMake/Ninja 或 `arm-none-eabi-gcc`。项目既有构建契约要求在 Ubuntu 虚拟机完成，因此本轮不能把 G1、SITL 或 FMUv6C build 标成通过。
