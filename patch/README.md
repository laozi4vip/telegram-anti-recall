# Anti-Recall 补丁文件

此目录包含 anti-recall 功能的所有修改文件，方便上游更新后手动替换。

## 文件对应源码路径

| 补丁文件 | 源码路径 |
|----------|----------|
| `ayu_settings.cpp` | `Telegram/SourceFiles/ayu/ayu_settings.cpp` |
| `ayu_settings.h` | `Telegram/SourceFiles/ayu/ayu_settings.h` |
| `messages_storage.cpp` | `Telegram/SourceFiles/ayu/data/messages_storage.cpp` |
| `messages_storage.h` | `Telegram/SourceFiles/ayu/data/messages_storage.h` |
| `settings_chats.cpp` | `Telegram/SourceFiles/ayu/ui/settings/settings_chats.cpp` |
| `history.cpp` | `Telegram/SourceFiles/history/history.cpp` |

## 使用方法

### 方式1：直接替换
将文件复制到 AyuGramDesktop 源码的对应路径下覆盖即可。

### 方式2：应用补丁
```bash
cd AyuGramDesktop
git apply patch/anti-recall.patch
```

## 注意事项

- 这些文件基于上游 commit `b25513a06f` 修改
- 如果上游更新了相同文件，直接替换可能丢失上游的新改动，建议使用 patch 方式并手动解决冲突
