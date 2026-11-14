# AyuGram 防撤回版 v7.2.9

基于 [AyuGram Desktop](https://github.com/AyuGram/AyuGramDesktop) 最新 `dev` 分支（`0f2a31bb42`）构建，在保留全部原版功能的基础上，增强了**消息防撤回**能力。

## 本次更新

### 新增：图片与视频防撤回

之前的版本只能保留被删除的**文字**消息，图片和视频被撤回后依然会消失。本版本完整支持媒体保留：

- **图片**：照片被撤回后保留在原位置，可正常预览、保存
- **视频**：普通视频、视频消息（圆形）、GIF 动图均可保留并播放
- **文件与语音**：文档、语音消息同样保留
- **相册分组**：同一条消息中的多张图片（grouped media）保持原有分组显示
- **可重新下载**：媒体以 Telegram 远程文件标识保存，只要服务器上文件仍存在即可再次下载

### 同步上游最新改动

合并了上游 `dev` 分支 **3409 个提交**，包含上游近期的功能改进与缺陷修复。

### 修复

- 修复合并后与上游新 API 的兼容问题（强类型 Peer ID 重载）
- 修复重注入消息的去重逻辑：原按「日期 + 文本」判重，纯媒体消息（无文字）会互相覆盖，现改为按消息 ID 精确判重

## 下载

| 文件 | 说明 |
|------|------|
| `ayu-anti-recall-setup-x64.7.2.9.exe` | **安装版**（推荐）。自动创建开始菜单与桌面快捷方式，支持从「应用和功能」卸载 |
| `AyuGram-anti-recall-portable-win64-7.2.9.zip` | **便携版**。解压后运行 `AyuGram\AyuGram.exe`，不写注册表，可放在 U 盘使用 |

仅提供 **Windows x64** 版本。若启动时提示缺少运行库，请安装 [Visual C++ 运行库](https://aka.ms/vs/17/release/vc_redist.x64.exe)。

## 使用方法

1. 打开 **AyuGram 设置 → 聊天设置（Chats）**
2. 确认 **「保留已删除消息」（Keep Deleted Messages in Chat）** 开关已开启（默认开启）
3. 此后对方删除的消息会以删除样式留在聊天中，重启客户端也不会消失

## 注意事项

- **仅对升级后新删除的消息生效**：旧版本已保存的消息没有媒体数据，仍按纯文本显示
- **一次性（TTL）媒体**：若对方发送的是阅后即焚内容，或服务器已清理该文件，即使有媒体标识也可能无法重新下载
- **其他媒体类型**：网页预览、投票、游戏、联系人等非图片/视频媒体仍按纯文本方式保留
- **纯本地实现**：所有删除消息仅保存在本地数据库（`tdata/ayudata.db`），不会向 Telegram 服务器发送任何数据，不影响服务器端记录
- **无应用内自动更新**：本包为自行构建版本，未包含 `Updater.exe`，因此不提供应用内自动更新；需要升级时请重新下载安装包

## 校验信息

| 文件 | SHA256 |
|------|--------|
| `AyuGram-anti-recall-portable-win64-7.2.9.zip` | `05FFD8223F5608BFEB7E69DF38F8AE5C4444E81B3F053B90BB8F6D7E3B7BE2E0` |
| `ayu-anti-recall-setup-x64.7.2.9.exe` | `5CC27EA51CED9B9374F5C01A743F234C07AB0123FAA3B430ABD04178CEF1CFDD` |

---

**上游项目**：[AyuGram/AyuGramDesktop](https://github.com/AyuGram/AyuGramDesktop) · **基础项目**：[telegramdesktop/tdesktop](https://github.com/telegramdesktop/tdesktop)
