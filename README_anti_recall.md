# AyuGramDesktop Anti-Recall 功能说明

本项目基于 [AyuGram/AyuGramDesktop](https://github.com/AyuGram/AyuGramDesktop) 修改，在保留原版所有功能的基础上，新增了**消息防撤回**相关功能。

---

## 新增功能

### 保留已删除消息（Keep Deleted Messages in Chat）

开启后，当对方在聊天中删除消息时，消息**不会从聊天界面消失**，而是以**删除状态**（半透明样式）继续显示，方便你事后回顾完整聊天记录。

**功能特点：**

- **实时显示**：对方删除消息时，消息立即变为删除状态显示，无需重启
- **重启后自动恢复**：程序重启后，已保存的删除消息会自动从本地数据库重新加载到聊天界面
- **发送方向正确**：重新加载的删除消息会根据原始发送者正确显示在左侧（对方消息）或右侧（自己的消息）
- **纯本地实现**：所有删除消息仅保存在本地数据库，不会向 Telegram 服务器发送任何数据，不影响服务器端数据，无账号风险
- **独立开关**：可在设置中随时开启或关闭

---

## 使用方法

1. 打开 AyuGram 设置 → 聊天设置（Chats）
2. 找到 **"保留已删除消息"**（Keep Deleted Messages in Chat）开关
3. 开启后，聊天中被删除的消息将以半透明样式继续显示

---

## 技术实现

### 核心修改文件

| 文件 | 修改内容 |
|------|----------|
| `ayu_settings.h` | 新增 `keepDeletedMessagesInChat` 设置项（默认开启） |
| `ayu_settings.cpp` | 新增设置项的读写逻辑 |
| `messages_storage.cpp` | 新增 `reinjectDeletedMessages()`：从本地数据库读取已删除消息并重新注入聊天界面；根据 `fromId` 设置 `MessageFlag::Outgoing`，确保消息发送方向正确 |
| `messages_storage.h` | 新增 `reinjectDeletedMessages()` 声明 |
| `settings_chats.cpp` | 在聊天设置页面新增 UI 开关 |
| `history.cpp` | 在 `addOlderSlice` 中调用 `reinjectDeletedMessages()` 后追加 `checkLocalMessages()`，确保重新注入的消息正确显示在聊天视图 |

### 实现原理

1. **拦截删除**：当收到服务器发来的消息删除通知时，不销毁消息对象，而是调用 `setDeleted()` 标记为已删除，并保存到本地数据库
2. **重新注入**：程序重启后，打开聊天时调用 `reinjectDeletedMessages()`，从数据库读取已删除消息，使用本地生成的 client-side ID 创建消息对象，注册到聊天历史中
3. **正确显示**：通过比较消息的 `fromId` 与当前用户 ID，设置 `MessageFlag::Outgoing` 标志，确保自己发送的消息显示在右侧，对方发送的消息显示在左侧
4. **聊天视图刷新**：在 `reinjectDeletedMessages()` 之后调用 `checkLocalMessages()`，将 client-side 消息插入到聊天视图的 blocks 中

### 安全性

- 重新注入的消息使用**本地 client-side ID**（负数），不会与服务器消息 ID 冲突
- 消息标记为 `MessageFlag::Local`，Telegram 客户端不会将其同步到服务器
- 服务器端的删除通知处理逻辑不受影响
- 所有操作完全在本地完成，与 Telegram 服务器隔离

---

## 从上游更新

本项目基于 AyuGramDesktop 上游仓库修改，当上游有新更新时：

```bash
# 拉取上游最新代码
git fetch origin

# 合并上游更新（保留我们的修改）
git rebase origin/dev
# 或使用 merge：git merge origin/dev

# 推送到我们的仓库
git push myfork dev:telegram-anti-recall
```

---

## 致谢

- 原项目：[AyuGram/AyuGramDesktop](https://github.com/AyuGram/AyuGramDesktop)
- Telegram Desktop：[telegramdesktop/tdesktop](https://github.com/telegramdesktop/tdesktop)
