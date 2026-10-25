<p align="center">
  <img src="../figures/logo.png" alt="kv-query logo" width="160px"/>
  <br>
  <a href="../../README.md">English</a> | 简体中文
</p>

一个 Windows 下使用的极简文本编辑器。只做一件事：**静默改文本，而不改动文件时间**。

## 特性

- **保存不改变文件时间**
- **自动识别常见编码**：UTF-8（BOM / 无 BOM）、UTF-16 LE / BE（BOM）、GBK
- **行尾风格**：
  - 纯 CRLF 文件保存后仍为 CRLF；空文件与不含换行的文件不受影响
  - 纯 LF、混合 CRLF/LF、纯 CR 文件保存后统一为 LF
- 单个 exe，无运行时依赖。

## 构建

依赖：Windows 7+, MinGW-w64。

在项目根目录用 PowerShell 或 CMD 运行：

```pwsh
.\build.bat
```

产物为 `dist\quietext.exe`，可以直接运行。

## 安装 / 卸载

```pwsh
.\install.bat     # 复制到 %LOCALAPPDATA%\quietext\bin 并加入用户 PATH
.\uninstall.bat   # 反向操作
```

安装后重开终端，即可在任意目录执行 `quietext`。

> CMD 运行时可以省略 `.\` 前缀

## 使用

安装后 CMD 与 PowerShell 均可直接调用：

```pwsh
quietext                # 若当前目录恰好一个文本文件则打开，否则弹选择框
quietext new.txt        # 不存在则新建，存在则打开
quietext D:\notes\a.md  # 打开指定文件
```

未安装时，也可直接运行构建产物：

```pwsh
.\dist\quietext.exe
```

## 快捷键

| 快捷键 | 操作 |
|---|---|
| Ctrl+N | 新建空白文档 |
| Ctrl+O | 打开 |
| Ctrl+S | 保存 |
| Ctrl+Shift+S | 另存为 |
| Ctrl+Q | 关闭当前文档，回到未命名空白状态，不退出程序 |
| Ctrl+Z / X / C / V / A | 撤销 / 剪切 / 复制 / 粘贴 / 全选 |

## 已知限制

- UTF-16 不带 BOM 无法自动识别，会按 ACP 解码而显示乱码
- 只读文件保存会失败，有弹框提示
