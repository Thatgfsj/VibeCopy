我是懒蛋我是懒蛋我是懒蛋是懒蛋是懒蛋懒蛋懒蛋蛋！

# VibeCopy 鼠标速复

快速复制工具 - 读取配置文件，显示按钮，点击即可复制文本到剪贴板。

## 功能

- 读取配置文件中的文本片段
- 显示为可视化按钮
- 点击按钮一键复制到剪贴板
- 支持窗口置顶
- 支持窗口缩放

## 配置文件格式

配置文件 `复制配置文件.txt` 支持以下格式：

```
显示名==实际文本
(显示名) 实际文本
纯文本（显示名=实际文本）
```

示例：
```
问候==你好，世界！
(邮箱) user@example.com
普通文本
(长文本) 这是一段很长的文本，点击即可复制
```

## 版本

### PowerShell 版本
- `vibe-copy.ps1` - 主程序
- `start-hidden.vbs` - 隐藏命令行窗口启动器

### C++ 版本
- `vibe-copy.cpp` - 源码
- `vibe-copy.exe` - 编译好的可执行文件

## 使用方法

1. 编辑 `复制配置文件.txt` 添加需要复制的文本
2. 运行程序：
   - PowerShell 版本：双击 `start-hidden.vbs`（隐藏命令行）
   - C++ 版本：双击 `vibe-copy.exe`
3. 点击按钮即可复制文本

## 编译 C++ 版本

```bash
clang++ -o vibe-copy.exe vibe-copy.cpp -mwindows -luser32 -lgdi32 -static
```

## 系统要求

- Windows 10/11
- PowerShell 5.1+（PowerShell 版本）
