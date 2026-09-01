# 在线系统辨识

电机效能估计状态：核心 RLS 为 `HOST_VERIFIED`，PX4 模块集成为 `IMPLEMENTED_UNVERIFIED`。

`EffectivenessEstimator` 定义 estimator 抽象，当前实现为 bounded RLS。它使用低通输入、持续激励门、健康基线冻结、遗忘因子、每电机独立有界 `lambda_i`、变化率限制和 confidence。解锁/落地/输入超时/电机数量改变会禁止更新或复位，避免把地面状态和初始化时序写入模型。

刚体转动观测明确计算：

`tau = I * omega_dot + omega x (I * omega)`

`FTC_EST_IXX/IYY/IZZ` 是静态标称惯量近似。当前没有足够可观测性证明在线惯量有效，因此 `inertia_valid=false`。质量字段与 CG offset 已进入 `ftc_model_status`，但质量估计和 CG 在线估计均为 `SKELETON`，分别保持 `mass_valid=false`、`cg_valid=false`，不会修改 MPC 或控制器参数。
