# Anti-Recall 补丁文件

此目录包含 anti-recall 功能的所有修改文件，方便上游更新后手动替换或重新应用。

## 文件对应源码路径

| 补丁文件 | 源码路径 |
|----------|----------|
| `ayu_settings.cpp` | `Telegram/SourceFiles/ayu/ayu_settings.cpp` |
| `ayu_settings.h` | `Telegram/SourceFiles/ayu/ayu_settings.h` |
| `messages_storage.cpp` | `Telegram/SourceFiles/ayu/data/messages_storage.cpp` |
| `messages_storage.h` | `Telegram/SourceFiles/ayu/data/messages_storage.h` |
| `settings_chats.cpp` | `Telegram/SourceFiles/ayu/ui/settings/settings_chats.cpp` |
| `history.cpp` | `Telegram/SourceFiles/history/history.cpp` |
| `ayu_mapper.cpp` | `Telegram/SourceFiles/ayu/utils/ayu_mapper.cpp` |
| `ayu_mapper.h` | `Telegram/SourceFiles/ayu/utils/ayu_mapper.h` |
| `message_history_item.cpp` | `Telegram/SourceFiles/ayu/ui/message_history/history_item.cpp` |

## 使用方法

### 方式1：直接替换
将文件复制到 AyuGramDesktop 源码的对应路径下覆盖即可。

### 方式2：应用补丁
```bash
cd AyuGramDesktop
git apply patch/anti-recall.patch
```

## 注意事项

- 这些文件基于上游 commit `0f2a31bb42`（upstream/dev）修改
- 如果上游更新了相同文件，直接替换可能丢失上游的新改动，建议使用 patch 方式并手动解决冲突
- `history.cpp` 与 `lang.strings` 属于上游文件，只包含少量 AyuGram 改动，优先用 patch 方式应用
