# quietext

一个 Windows 下的极简文本编辑器。目标只有一个：**改文本，但不动文件时间**。

## 特性

- **保存不改变文件时间**
  - 读取时恢复 LastAccessTime（读操作会刷新它）
  - 保存时在同一句柄上恢复 CreationTime / LastAccessTime / LastWriteTime

- **自动识别编码**：UTF-8（BOM / 无 BOM）、UTF-16 LE / BE（BOM）、GBK（系统 ACP）
  - 保存时按原编码写回；无法表示时提示升级为 UTF-8 BOM

- **行尾风格**：
  - 纯 CRLF 文件保存后仍为 CRLF
  - 纯 LF、混合 CRLF/LF、纯 CR(老 Mac) 文件保存后统一为 LF
  - 空文件与不含换行的文件不受影响

- 上下滚动、鼠标滚轮、选择、剪贴板由系统 `EDIT` 控件提供，行为与记事本一致。
- 单 exe，无运行时依赖。

## 构建

依赖：MinGW-w64，`g++` 在 PATH 中。在项目根目录运行：

```bat
build.bat
```

PowerShell 里也可写 `./build.bat`。

产物：`dist\quietext.exe`，可以直接运行。

## 安装 / 卸载

```bat
install.bat     :: 复制到 %LOCALAPPDATA%\quietext\bin 并加入用户 PATH
uninstall.bat   :: 反向操作
```

安装后重开终端，即可在任意目录执行 `quietext`。

## 使用

```bat
quietext                    :: 当前目录恰好一个文本文件则打开；否则弹选择框
quietext new.txt            :: 不存在则新建（首次保存才落盘），存在则打开
quietext D:\notes\a.md      :: 打开指定文件
```

## 快捷键

| 快捷键 | 动作 |
|---|---|
| Ctrl+N | 新建空白文档 |
| Ctrl+O | 打开… |
| Ctrl+S | 保存 |
| Ctrl+Shift+S | 另存为… |
| Ctrl+Q | 关闭当前文档，回到未命名空白状态，不退出程序 |
| Ctrl+Z / X / C / V / A | 撤销 / 剪切 / 复制 / 粘贴 / 全选 |

## 已知限制

- UTF-16 不带 BOM 无法自动识别（会按 ACP 解码，显示乱码）
- 只读文件保存会失败（弹框提示，不会自动改属性）
