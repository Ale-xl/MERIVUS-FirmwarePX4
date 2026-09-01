# 故障诊断

状态：`IMPLEMENTED_UNVERIFIED`。

`motor_health_monitor` 对每个电机发布 fault probability、confidence、persistence 和 fault type。分类集合覆盖 NONE、未知推进退化、桨损伤、电机退化、电机停转、ESC/供电故障、间歇推进故障、机械不平衡、外部扰动和模型失配。

证据来自电机指令、角速度/角加速度、模型残差、机动强度、线加速度高频分量以及可选 ESC online/failure/RPM/current/voltage/temperature 数据。当前实现直接使用 online、failure 和 RPM 强证据；电流、电压、温度字段保留在输入契约中，阈值模型待数据标定。

分类原则是宁可输出 UNKNOWN，也不在证据不足时猜测具体部件。无 ESC 遥测时设置 `imu_only=true` 并降低 fault confidence。外部扰动和模型失配只在没有稳定局部故障证据时输出。

尚缺：SITL 混淆矩阵、ULog 阈值标定、不同桨/电机/ESC 的实测特征库，以及 ESC 地址到 allocator actuator 的通用映射验证。
