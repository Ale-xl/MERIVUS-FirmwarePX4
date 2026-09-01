# 基线快照

## 冻结结果

- 仓库：`E:\MERIVUS\FirmwarePX4`
- 冻结时间：2026-08-31（Asia/Shanghai）
- 原始分支：`main`
- 原始 HEAD：`3ec2f9f2c3e73f7639d2650bbd367ccef34b0163`
- 原始上游：`origin/main`，审计时与 HEAD 一致
- dirty 状态：干净；无暂存、未暂存或未跟踪文件
- 备份分支：`backup/px4-v1.14-current-20260831`
- 不可移动恢复标签：`archive/px4-v1.14-current-20260831`
- 开发分支：`research/extreme-control-v1`
- 产品版本标识：`v1.14.0-1.0.0-25-g3ec2f9f2c3`
- `package.xml` 版本：`1.14.0`
- FMUv6C 构建目标：`px4_fmu-v6c_default`
- SITL 构建目标：`px4_sitl_default`

annotated tag 的目标已用 `git cat-file` 和 `git rev-list -n 1` 验证，精确指向上述 HEAD。仓库采用导入式产品历史，与 `upstream/release/1.14` 没有共同祖先，不能用 merge-base 推断官方基线提交。

## 子模块状态

Git 索引记录 17 个 gitlink。审计时 9 个子模块已初始化且 HEAD 与父仓库记录一致、工作树干净；另外 8 个路径未带子模块 Git 元数据。未执行子模块更新，以免改变冻结状态。

## 恢复方法

先保存当前任何未提交工作，再选择以下方式之一：

```sh
# 在新恢复分支中打开冻结状态，最不容易误伤现有分支
git switch -c restore/px4-v1.14-current archive/px4-v1.14-current-20260831

# 或直接查看只读备份分支
git switch backup/px4-v1.14-current-20260831
```

原始提交同时仍由审计时的 `main`、`origin/main` 和上述两个本地恢复锚点引用。本项目不要求用 reset、clean 或覆盖工作树来恢复。

