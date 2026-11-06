# 发布到 GitHub

仓库内容已经可以直接发布。正式发布前，可以按需修改 `LICENSE` 中的版权名称。

## 使用 GitHub CLI

在仓库根目录运行：

```bash
git init -b main
git config user.name "YOUR_PUBLIC_NAME"
git config user.email "YOUR_GITHUB_NOREPLY_EMAIL"
git add .
git commit -m "Initial release: v1.0.0"
gh auth login
gh repo create mac-ac-disablesleep --public --source=. --remote=origin --push
git tag -a v1.0.0 -m "mac-ac-disablesleep v1.0.0"
git push origin v1.0.0
```

把 `YOUR_PUBLIC_NAME` 替换成愿意公开显示的名称，把 `YOUR_GITHUB_NOREPLY_EMAIL` 替换成 GitHub **Settings → Emails** 页面提供的 noreply 邮箱，避免提交记录公开真实邮箱。

然后在 GitHub 的 **Releases** 页面基于 `v1.0.0` 创建 Release，可直接使用 `CHANGELOG.md` 中的 `1.0.0` 内容作为发行说明。

## 使用 GitHub 网页

1. 在 GitHub 新建名为 `mac-ac-disablesleep` 的公开仓库，不要自动添加 README 或许可证。
2. 在本地仓库根目录依次运行：

```bash
git init -b main
git config user.name "YOUR_PUBLIC_NAME"
git config user.email "YOUR_GITHUB_NOREPLY_EMAIL"
git add .
git commit -m "Initial release: v1.0.0"
git remote add origin https://github.com/YOUR_USERNAME/mac-ac-disablesleep.git
git push -u origin main
git tag -a v1.0.0 -m "mac-ac-disablesleep v1.0.0"
git push origin v1.0.0
```

3. 把命令中的 `YOUR_USERNAME` 替换成实际 GitHub 用户名。
4. 建议在仓库 **Settings → Security** 中启用 Private vulnerability reporting。

## 发布前检查

```bash
make clean
make check
git status
```

确认没有密码、设备序列号、私人日志或其他凭据后再推送。
