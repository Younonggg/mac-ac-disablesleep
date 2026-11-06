# mac-ac-disablesleep

根据电源状态控制 macOS 笔记本合盖休眠，并在插电时保持网络在线。

[English](README.md)

`mac-ac-disablesleep` 会在接通电源时阻止系统休眠。即使设备是在已经合盖、使用电池休眠后才接通电源，也会唤醒并恢复网络。拔掉电源后，它会恢复 macOS 原有的合盖休眠行为。

## 行为

| 电源状态 | `disablesleep` | 附加断言 | 合盖行为 |
| --- | ---: | --- | --- |
| 接通电源 | `1` | 阻止系统休眠并保持网络客户端活跃 | 保持运行和在线 |
| 使用电池 | `0` | 全部释放 | 使用设备原有休眠设置 |

本项目不会修改设备原有的电池休眠计时。

## 为什么需要后台服务

`disablesleep` 是系统级全局设置，不能仅依靠静态的 `pmset -b` 和 `pmset -c` 配置可靠地保存两套值。本项目通过一个小型 root LaunchDaemon 监听 IOKit 电源变化，并执行以下固定命令之一：

```text
/usr/bin/pmset -a disablesleep 1  # 插电
/usr/bin/pmset -a disablesleep 0  # 电池或其他非 AC 电源
```

插电时还会：

- 建立 `PreventSystemSleep` 和 `NetworkClientActive` 断言；
- 建立 30 秒 `UserIsActive` 断言，将暗唤醒提升为完整唤醒，让 Wi-Fi 和远程控制软件恢复连接；
- 启用 `acwake 1`，使合盖休眠中的 Mac 能响应电源变化。

## 要求

- macOS 笔记本电脑；
- 管理员账号；
- Apple Command Line Tools。

当前版本已在 Apple Silicon Mac 上完成实机验证。安装程序会在目标 Mac 上编译原生架构程序。

## 安装

如果尚未安装 Command Line Tools：

```bash
xcode-select --install
```

克隆或下载仓库后运行：

```bash
./scripts/install.sh
```

安装程序会通过标准 `sudo` 提示请求管理员权限，不会保存密码。无需重启。

## 验证

```bash
./scripts/status.sh
```

插电时应看到：

```text
SleepDisabled  1
PreventSystemSleep  1
NetworkClientActive  1
```

合盖测试：

1. 拔掉电源并合盖；
2. 确认设备离线；
3. 保持合盖并接通电源；
4. 等待约 15–30 秒，确认 Wi-Fi 和远程服务重新上线。

## 卸载

```bash
./scripts/uninstall.sh
```

这会停止服务、删除安装的两个系统文件，并恢复 `disablesleep 0`。为了避免覆盖设备原有偏好，默认保留当前 `acwake` 设置。如需同时关闭：

```bash
./scripts/uninstall.sh --disable-acwake
```

## 安全和限制

- Mac 合盖持续运行时会产生热量，插电状态下请勿放入电脑包、抽屉等不利于散热的位置。
- 远程控制软件仍需设置为自动启动，并获得必要的后台运行及网络权限。
- 其他电源管理软件可能覆盖 `pmset` 或创建冲突断言。
- 完全关机后，后台服务无法运行或响应电源变化。
- `disablesleep` 没有记录在公开的 `pmset` 手册中，未来 macOS 版本可能改变其行为。

## 开发

```bash
make check
```

参见 [CONTRIBUTING.md](CONTRIBUTING.md) 和 [SECURITY.md](SECURITY.md)。

## 许可证

[MIT](LICENSE)
