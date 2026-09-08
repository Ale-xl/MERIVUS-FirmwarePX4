# FTC 电机效能分级复测

本流程只用于 PX4 SITL。`FTC_CA_EN=0`、`FTC_REC_ACT=0` 是不可放宽的安全锁；不得在真实硬件上执行故障注入。

## QUICK RETEST CARD

1. **Verify Version**：记录 branch、HEAD、工作区状态和本轮时间戳。
2. **Build**：核对当前 SITL/FMUv6C/单测 PASS 记录；仅在源码变化时重建。
3. **Start New SITL**：每场启动全新进程，不沿用异常会话。
4. **Apply Safe Config**：应用安全参数全集，确认两个 ACTIVE 开关均为 0。
5. **Check Modules**：五个模块 `status` 均为运行状态。
6. **Takeoff**：起飞至足够高度，稳定保持 Position/Loiter。
7. **Pass Baseline Gate**：逐项核对六个状态 topic；任一失败即禁止注入。
8. **Apply Injection**：仅执行当前独立场景的完整命令。
9. **Capture Evidence**：保存参数、status、listener、ULog 和必要截图。
10. **Disable Simulation**：先执行 `param set FTC_SIM_EN 0`，再恢复中性参数。
11. **Land / Disarm**：安全降落并解除武装，不在空中重启模块。
12. **Confirm Recovery Reset**：确认 Recovery 回到 `MONITORING` 且 ACTIVE 为 false。
13. **Save ULog**：仅归档会话开始后生成的日志，保留原件并计算 SHA-256。
14. **Stop SITL**：证据落盘后结束当前进程。
15. **Start Next Independent Session**：仅在本级 PASS 后开始下一档全新会话。

## 1. 参数契约

| 参数 | 类型/范围 | 默认值 | 单位 | 运行时语义 |
| --- | --- | ---: | --- | --- |
| `FTC_MON_EN` | bool，0/1 | 0 | 无 | 启用监测链；启动脚本仅在启动时根据它拉起五个 FTC 模块，参数元数据标记需重启生效。 |
| `FTC_CA_SHADOW` | bool，0/1 | 0 | 无 | 仅计算并发布影子分配结果和控制裕度，不连接实际执行器输出。 |
| `FTC_CA_EN` | bool，0/1 | 0 | 无 | 预留的主动接管开关；当前实现未连接接管路径，复测仍必须保持 0。 |
| `FTC_IMPACT_EN` | bool，0/1 | 0 | 无 | 启用冲击检测。 |
| `FTC_LOC_EN` | bool，0/1 | 0 | 无 | 启用失控风险检测。 |
| `FTC_REC_EN` | bool，0/1 | 0 | 无 | 启用恢复状态机和候选量生成。 |
| `FTC_REC_ACT` | bool，0/1 | 0 | 无 | 预留的主动恢复开关；当前实现不输出干预，复测必须保持 0。 |
| `FTC_SIM_EN` | bool，0/1 | 0 | 无 | 仅 POSIX/SITL 仿真桥的电机效能注入总开关，不作用于真实硬件。 |
| `FTC_SIM_MOT` | int，1–12 | 1 | 无 | 用户侧为 1-based；`1` 表示 M1，内部消息 `motor_index=0`。 |
| `FTC_SIM_EFF` | float，0–1 | 1.0 | 比例 | 直接乘到目标电机控制量；1.0/0.90/0.80/0.70 分别表示 100%/90%/80%/70% 效能。 |
| `FTC_SIM_RAMP` | float，0–120 | 0 | s | 完成满量程效能变化所需时间，每周期最大变化为 `dt / FTC_SIM_RAMP`；0 为立即应用。 |
| `FTC_SIM_INT` | float，0–60 | 0 | s | 正值时按半周期在目标效能和 1.0 间交替；0 禁用间歇模式。 |

参数来源：`motor_health_monitor_params.c`、`simulator_mavlink_params.c`；注入换算来源：`SimulatorMavlink.cpp`。本轮目标固定为界面 M1，因此 `FTC_SIM_MOT=1`，对应内部索引 0。

## 2. 安全参数全集

在每个新 SITL 会话中核对以下全集：

```sh
param set FTC_MON_EN 1
param set FTC_CA_SHADOW 1
param set FTC_CA_EN 0
param set FTC_IMPACT_EN 1
param set FTC_LOC_EN 1
param set FTC_REC_EN 1
param set FTC_REC_ACT 0
param set FTC_SIM_EN 0
param set FTC_SIM_MOT 1
param set FTC_SIM_EFF 1.0
param set FTC_SIM_RAMP 0
param set FTC_SIM_INT 0
```

随后逐项执行 `param show` 留存输出。若 `FTC_MON_EN` 是在本进程启动后从 0 改为 1，不要依赖偶然的手工启动顺序；保存参数并结束该进程，再启动一个全新 SITL，让 `rc.mc_apps` 按统一入口启动模块。

```sh
param show FTC_MON_EN
param show FTC_CA_SHADOW
param show FTC_CA_EN
param show FTC_IMPACT_EN
param show FTC_LOC_EN
param show FTC_REC_EN
param show FTC_REC_ACT
param show FTC_SIM_EN
param show FTC_SIM_MOT
param show FTC_SIM_EFF
param show FTC_SIM_RAMP
param show FTC_SIM_INT
```

任何注入前若 `FTC_CA_EN` 或 `FTC_REC_ACT` 不为 0：`TEST BLOCKED`。

安全不变量：

- `FTC_CA_SHADOW=1` 只允许影子计算，不能改变飞控执行器输出。
- `FTC_CA_EN=0` 和 `FTC_REC_ACT=0` 在全部场景中保持不变。
- 任何场景退出时都恢复 `FTC_SIM_EN=0`、`FTC_SIM_EFF=1.0`、`FTC_SIM_RAMP=0`、`FTC_SIM_INT=0`。

## 3. 模块启动闭环

`ROMFS/px4fmu_common/init.d/rc.mc_apps` 在 `FTC_MON_EN=1` 时按统一入口启动：

```sh
motor_health_monitor status
ftc_control_monitor status
ftc_extreme_state_monitor status
ftc_recovery status
ftc_supervisor status
```

预期五项均为 `running/ACTIVE`。不要重复执行 `start` 来掩盖启动脚本问题；若新 SITL 仍有模块缺失，停止复测并保存启动日志。

## 4. 注入前基线门

只有以下条件同时成立，才允许执行某场景最后一条 `param set FTC_SIM_EN 1`：

1. 飞行器已起飞，在足够高度稳定保持 Position/Loiter；禁止起飞前、贴地或正在降落时注入。
2. `ftc_model_status.valid=true` 且 `effectiveness_valid=true`。
3. `ftc_model_status.motor_count=4`；四个有效电机的 `motor_confidence[]` 均不低于 `FTC_CONF_MIN=0.60`。
4. `ftc_model_status.excitation` 已形成有效激励，`model_quality` 和 `rotational_residual` 稳定，无突跳或持续恶化。
5. `motor_health_status.state=VALID`、`model_valid=true`、`motor_count=4`。
6. `motor_health_status.degraded_mask=0`、`failed_mask=0`，四电机效能/健康接近正常基线。
7. `ftc_control_authority.valid=true`、`matrix_valid=true`、`state=FULL_CONTROL`、`saturated_mask=0`。
8. `ftc_control_authority` 的 roll/pitch/yaw/thrust authority、minimum attitude authority 和 actuator headroom 稳定，无持续下降。
9. `ftc_extreme_state.valid=true`、`impact_detected=false`、`hard_landing=false`、`impact_type=NONE`、`loc_state=LOC_NORMAL`。
10. `ftc_recovery_status.state=MONITORING`、`active=false`、`candidate_valid=false`、`intervention_enabled=false`、`trigger_mask=0`。
11. `ftc_system_status.state=NORMAL`、`monitor_enabled=true`、`model_valid=true`、`fault_confirmed=false`、`degraded_mask=0`、`failed_mask=0`、`control_authority_valid=true`、`intervention_enabled=false`、`reason_mask=0`。
12. 当前 ULog 已实际记录第 7 节列出的预期 FTC topics；只有 listener 可见而 ULog 缺失时停止。

关键取证命令：

```sh
listener ftc_model_status 1
listener motor_health_status 1
listener ftc_control_authority 1
listener ftc_extreme_state 1
listener ftc_recovery_status 1
listener ftc_system_status 1
listener ftc_simulation_status 1
```

`valid=false`、置信度不足、模型残差异常、状态不是 NORMAL/MONITORING/FULL_CONTROL、任一 mask 非零或日志不完整，都属于硬停止条件。先关闭注入，再安全降落并保留证据。

## 5. Recovery 新会话复位

源码只在以下条件同时成立时把 `ABORTED` 或 `FAILED` 复位到 `MONITORING`：车辆状态和着陆状态时间戳新鲜、已解除武装、已落地、`trigger_mask=0`。两个终态都没有依赖等待时长的自动超时复位。

正常复位顺序：

```sh
param set FTC_SIM_EN 0
param set FTC_SIM_EFF 1.0
listener vehicle_status 1
listener vehicle_land_detected 1
listener ftc_recovery_status 1
ftc_recovery status
```

确认已落地、解除武装和 trigger 清零后，等待下一次状态机周期；应回到 `MONITORING`。若未复位，优先结束整个异常 SITL，启动新会话。

只有在地面、已解除武装、注入已关闭、五个上游模块状态正常且必须保留当前 SITL 进行诊断时，才允许最后手段重启 Recovery：

```sh
ftc_recovery stop
ftc_recovery start
ftc_recovery status
listener ftc_recovery_status 1
```

不得在空中重启模块，也不得用重启绕过持续存在的 trigger 或 inhibit 根因。

## 6. 五组场景精确命令

每一场必须使用独立的新 SITL、独立 ULog，并从第 2–4 节重新验证。不能在同一异常会话中直接改效能值进入下一等级。

固定流程：`NEW SITL PROCESS → NEW FLIGHT → SAFE CONFIG → TAKEOFF → STABLE POSITION/LOITER → BASELINE GATE → INJECTION → CAPTURE → SIM OFF → LAND → DISARM → RESET CONFIRM → SAVE ULOG → STOP SITL`。

### NORMAL

```sh
param set FTC_CA_EN 0
param set FTC_REC_ACT 0
param set FTC_SIM_EN 0
param set FTC_SIM_MOT 1
param set FTC_SIM_EFF 1.0
param set FTC_SIM_RAMP 0
param set FTC_SIM_INT 0
```

### EFF 1.00

```sh
param set FTC_CA_EN 0
param set FTC_REC_ACT 0
param set FTC_SIM_EN 0
param set FTC_SIM_MOT 1
param set FTC_SIM_EFF 1.0
param set FTC_SIM_RAMP 0
param set FTC_SIM_INT 0
param set FTC_SIM_EN 1
```

### EFF 0.90

```sh
param set FTC_CA_EN 0
param set FTC_REC_ACT 0
param set FTC_SIM_EN 0
param set FTC_SIM_MOT 1
param set FTC_SIM_EFF 0.90
param set FTC_SIM_RAMP 0
param set FTC_SIM_INT 0
param set FTC_SIM_EN 1
```

### EFF 0.80

```sh
param set FTC_CA_EN 0
param set FTC_REC_ACT 0
param set FTC_SIM_EN 0
param set FTC_SIM_MOT 1
param set FTC_SIM_EFF 0.80
param set FTC_SIM_RAMP 0
param set FTC_SIM_INT 0
param set FTC_SIM_EN 1
```

### EFF 0.70

```sh
param set FTC_CA_EN 0
param set FTC_REC_ACT 0
param set FTC_SIM_EN 0
param set FTC_SIM_MOT 1
param set FTC_SIM_EFF 0.70
param set FTC_SIM_RAMP 0
param set FTC_SIM_INT 0
param set FTC_SIM_EN 1
```

每场结束统一恢复：

```sh
param set FTC_SIM_EN 0
param set FTC_SIM_MOT 1
param set FTC_SIM_EFF 1.0
param set FTC_SIM_RAMP 0
param set FTC_SIM_INT 0
param set FTC_CA_EN 0
param set FTC_REC_ACT 0
```

## 7. Logger 与发布条件闭环

`logged_topics.cpp` 已把以下 FTC topics 作为常规订阅加入日志；“已订阅”不等于“必然有发布者数据”，复测必须同时满足右栏条件。

| Topic | Logger 间隔 | 实际发布条件 | 五场预期 |
| --- | ---: | --- | --- |
| `motor_health_status` | 20 ms | `motor_health_monitor` 运行；模型是否有效由飞行状态、激励和置信度决定。 | 必须存在 |
| `ftc_model_status` | 20 ms | 与电机健康模型同源，由 `motor_health_monitor` 发布。 | 必须存在 |
| `ftc_effectiveness_matrix` | 1000 ms | Control Allocator 已形成并更新有效效能矩阵。 | 必须存在 |
| `ftc_allocation_shadow` | 20 ms | `FTC_CA_SHADOW=1`、矩阵有效且影子输入新鲜。 | 空中基线后必须存在 |
| `ftc_control_authority` | 20 ms | `ftc_control_monitor` 运行；条件不足时也会低频发布 invalid 状态。 | 必须存在且基线 valid |
| `ftc_extreme_state` | 20 ms | `ftc_extreme_state_monitor` 运行。 | 必须存在 |
| `ftc_recovery_status` | 20 ms | `ftc_recovery` 运行。 | 必须存在 |
| `ftc_system_status` | 100 ms | `ftc_supervisor` 运行。 | 必须存在 |
| `ftc_simulation_status` | 20 ms | POSIX/SITL `SimulatorMavlink` 正在发送电机控制；关闭注入时也应报告中性状态。 | 必须存在 |

还应保留 `actuator_motors`、`vehicle_torque_setpoint`、`vehicle_thrust_setpoint`、`control_allocator_status`、姿态、角速度、角加速度、本地高度和垂直速度，用于解释效能估计、饱和和 LOC 的先后关系。

## 8. ULog 归档命令

在 Ubuntu Shell 为整轮复测建立唯一目录；若目录已存在，命令会失败，禁止覆盖旧证据：

```bash
RETEST_ROOT=/home/cwkj/MERIVUS/test-artifacts/retest
RETEST_RUN_DIR="$RETEST_ROOT/$(date +%Y%m%d-%H%M%S)"
test ! -e "$RETEST_RUN_DIR"
mkdir -p -- "$RETEST_RUN_DIR"/{NORMAL,EFF100,EFF90,EFF80,EFF70}
printf '%s\n' "$RETEST_RUN_DIR" | tee "$RETEST_RUN_DIR/RUN_DIRECTORY.txt"
git -C /home/cwkj/MERIVUS/FirmwarePX4 branch --show-current > "$RETEST_RUN_DIR/branch.txt"
git -C /home/cwkj/MERIVUS/FirmwarePX4 rev-parse HEAD > "$RETEST_RUN_DIR/head.txt"
git -C /home/cwkj/MERIVUS/FirmwarePX4 status --short > "$RETEST_RUN_DIR/status.txt"
export RETEST_RUN_DIR
```

每次启动对应场景前在 Ubuntu Shell 记录：

```bash
SESSION_START_EPOCH=$(date +%s)
printf '%s\n' "$SESSION_START_EPOCH"
```

场景结束、PX4 已完成 ULog 写入后，使用下列函数。它只选择会话开始后生成的最新 `.ulg`，校验源文件非空，并拒绝覆盖目标文件：

```bash
find /home/cwkj/MERIVUS/FirmwarePX4/build/px4_sitl_default/rootfs/log \
    -type f -name '*.ulg' -newermt "@$SESSION_START_EPOCH" \
    -printf '%T@ size=%s path=%p\n' | sort -nr
```

```bash
archive_ftc_ulog() {
    local test_name="$1"
    local target_name="$2"
    local session_start_epoch="$3"
    local source_root=/home/cwkj/MERIVUS/FirmwarePX4/build/px4_sitl_default/rootfs/log
    local destination_dir="$RETEST_RUN_DIR/$test_name"
    local latest_ulg
    latest_ulg=$(find "$source_root" -type f -name '*.ulg' -newermt "@$session_start_epoch" -printf '%T@ %p\n' |
        sort -nr | head -n 1 | cut -d' ' -f2-)
    test -n "$latest_ulg"
    test -s "$latest_ulg"
    local destination="$destination_dir/$target_name"
    test ! -e "$destination"
    cp -- "$latest_ulg" "$destination"
    {
        printf 'source=%s\ndestination=%s\n' "$latest_ulg" "$destination"
        stat --printf='source_size=%s\nsource_mtime=%y\n' "$latest_ulg"
        stat --printf='destination_size=%s\ndestination_mtime=%y\n' "$destination"
    } | tee "$destination_dir/ULOG_SOURCE_METADATA.txt"
    sha256sum "$destination" | tee "$destination.sha256"
}
```

五场分别调用；第三个参数必须是该场启动前记录的实际 epoch：

```bash
archive_ftc_ulog NORMAL FTC_NORMAL.ulg "$SESSION_START_EPOCH"
archive_ftc_ulog EFF100 FTC_EFF100.ulg "$SESSION_START_EPOCH"
archive_ftc_ulog EFF90 FTC_EFF90.ulg "$SESSION_START_EPOCH"
archive_ftc_ulog EFF80 FTC_EFF80.ulg "$SESSION_START_EPOCH"
archive_ftc_ulog EFF70 FTC_EFF70.ulg "$SESSION_START_EPOCH"
```

每次只能调用与当前场景匹配的一行，并在进入下一场前重新记录 `SESSION_START_EPOCH`。若 `find` 没找到新日志、找到多个时间异常候选、文件大小为零或目标已存在，立即停止人工核对，不得改用旧日志凑数。

## 9. 证据矩阵

| 场景 | ULog | 命令/机器证据 | GroundStation 视觉证据 | 最低取证时点 |
| --- | --- | --- | --- | --- |
| NORMAL | `NORMAL/FTC_NORMAL.ulg` + SHA-256 | HEAD、时间戳、SAFE CONFIG、模块 status、六个 baseline listener | `NORMAL/10_GS_BASELINE.png`、必要 PX4 console | 稳定基线 |
| EFF 1.00 | `EFF100/FTC_EFF100.ulg` + SHA-256 | BEFORE/AFTER/END 的 model、motor、authority、extreme、recovery、system、simulation status | `11_EFF100_BEFORE.png`、`12_EFF100_AFTER.png`、必要 END 画面 | 注入前、注入后、结束 |
| EFF 0.90 | `EFF90/FTC_EFF90.ulg` + SHA-256 | BEFORE/T0+1 s/END；重点含目标 E/H/confidence/fault probability、其他电机、authority、LOC、Recovery | `13_EFF90_BEFORE.png`、`14_EFF90_T0_PLUS_1S.png`、`15_EFF90_END.png` | 注入前、T0+1 s、结束 |
| EFF 0.80 | `EFF80/FTC_EFF80.ulg` + SHA-256 | BEFORE/T0+1 s/END；字段要求同 EFF 0.90 | `16_EFF80_BEFORE.png`、`17_EFF80_T0_PLUS_1S.png`、`18_EFF80_END.png` | 注入前、T0+1 s、结束 |
| EFF 0.70 | `EFF70/FTC_EFF70.ulg` + SHA-256 | BEFORE/FIRST_ABNORMAL/END；重点含 E/H/confidence/residual/saturation/authority/LOC/Recovery | `19_EFF70_BEFORE.png`、`20_EFF70_FIRST_ABNORMAL.png`、`21_EFF70_END.png` | 注入前、首个异常、结束 |

GroundStation 截图必须显示实时、非 stale 的 FTC 字段和对应飞行状态；仅有界面布局、N/A 或“数据已过期”不构成通过证据。每场还应保存参数输出、五模块 status、基线门 listener 输出、注入后 listener 输出、停止原因和异常时间戳。

## 10. 分级判定与停止规则

每一级 PASS 必须同时满足：

- `ftc_simulation_status` 的目标、实际效能和内部电机索引与命令一致；
- M1 估计趋势与注入等级一致，其他电机没有共同虚假退化；
- confidence、excitation、model quality 和 residual 能解释模型有效性变化；
- authority、饱和、LOC、Recovery 和姿态响应的时间顺序可由 ULog 重现；
- NORMAL 与 EFF 1.00 不产生持续退化或故障；
- ULog、命令文本、视觉证据和 SHA-256 全部存在且属于同一 Git HEAD/工作区快照。

以下任一情况立即执行注入恢复，安全降落并终止后续等级：

- 基线门任一项失败；
- GroundStation 数据 stale、MAVLink 字段缺失或语义与 uORB 不一致；
- 非目标电机共同下降、模型 invalid、confidence 崩溃或 residual 无法解释；
- 控制裕度显著恶化、持续饱和、LOC 升级、Recovery 离开预期观察态；
- ULog topic 缺失、日志无法唯一定位或证据无法和当前会话绑定；
- 飞行姿态、高度或操纵状态不再满足安全条件。

本文件只定义复测前契约和取证流程，不代表五组人工飞行已经执行或通过。
