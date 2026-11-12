# Telegram-anti-recall


![AyuGram Logo](.github/AyuGram.png) ![AyuChan](.github/AyuChan.png)

本项目基于 [AyuGram/AyuGramDesktop](https://github.com/AyuGram/AyuGramDesktop) 修改，在保留原版所有功能的基础上，新增了**消息防撤回**相关功能。

---

## 新增功能

### 保留已删除消息（Keep Deleted Messages in Chat）

开启后，当对方在聊天中删除消息时，消息**不会从聊天界面消失**，而是以**删除状态**继续显示，方便回顾完整聊天记录。

**功能特点：**

- **实时显示**：对方删除消息时，消息立即变为删除状态显示，无需重启
- **重启后自动恢复**：程序重启后，已保存的删除消息会自动从本地数据库重新加载到聊天界面
- **发送方向正确**：重新加载的删除消息会根据原始发送者正确显示在左侧（对方消息）或右侧（自己的消息）
- **纯本地实现**：所有删除消息仅保存在本地数据库，不会向 Telegram 服务器发送任何数据，不影响服务器端数据
- **独立开关**：可在设置中随时开启或关闭

---

## 使用方法

1. 打开 AyuGram 设置 → 聊天设置（Chats）
2. 找到 **"保留已删除消息"**（Keep Deleted Messages in Chat）开关（默认开启）
3. 开启后，聊天中被删除的消息将以删除样式继续显示

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

## 原版功能

AyuGramDesktop 原版功能包括：

- Full ghost mode (flexible)
- Messages history
- Anti-recall
- Font customization
- Streamer mode
- Local Telegram Premium
- Translator
- Media preview & quick reaction on force click (macOS)
- Enhanced appearance

更多详情见 [AyuGram 官方文档](https://docs.ayugram.one/desktop/)。

<h3>
<details>
<summary>Preview</summary>
<table>
<tr>
<td><img src='.github/demos/demo1.png' width='268' alt='Preferences'></td>
<td><img src='.github/demos/demo2.png' width='268' alt='AyuGram Options'></td>
<td><img src='.github/demos/demo3.png' width='268' alt='Message Filters'></td>
</tr>
<tr>
<td><img src='.github/demos/demo4.png' width='268' alt='Appearance'></td>
<td><img src='.github/demos/demo5.png' width='268' alt='Chats'></td>
</tr>
</table>
</details>
</h3>

---

## 下载与构建

### Windows

#### Official

You can download prebuilt Windows binary from [Releases tab](https://github.com/AyuGram/AyuGramDesktop/releases) or from
the [Telegram channel](https://t.me/AyuGramReleases).

#### Winget

```bash
winget install RadolynLabs.AyuGramDesktop
```

#### Scoop

```bash
scoop bucket add extras
scoop install ayugram
```

#### Self-built

Follow [official guide](https://github.com/AyuGram/AyuGramDesktop/blob/dev/docs/building-win.md) if you want to
build by yourself.

Make sure you have these components installed with VS Build Tools:
- C++ MFC latest (x86 & x64)
- C++ ATL latest (x86 & x64)
- latest Windows 11 SDK

### macOS

#### Official

You can download prebuilt macOS package from [Releases tab](https://github.com/AyuGram/AyuGramDesktop/releases).

#### Homebrew

```bash
brew install --cask ayugram
```

#### Self-built

Follow the [official guide](https://github.com/AyuGram/AyuGramDesktop/blob/dev/docs/building-mac.md).

### Arch Linux

#### From source (recommended)

Install `ayugram-desktop` from [AUR](https://aur.archlinux.org/packages/ayugram-desktop).

#### Prebuilt binaries

Install `ayugram-desktop-bin` from [AUR](https://aur.archlinux.org/packages/ayugram-desktop-bin).

Note: these binaries aren't officially maintained by us.

### NixOS

#### Flake (recommended)

Install `ayugram-desktop` from [ndfined-crp/ayugram-desktop](https://github.com/ndfined-crp/ayugram-desktop)

#### Nixpkgs

Install `ayugram-desktop` from [nixpkgs](https://search.nixos.org/packages?channel=unstable&show=ayugram-desktop)

### ALT Linux

[Sisyphus](https://packages.altlinux.org/en/sisyphus/srpms/ayugram-desktop/)

### Gentoo Linux

See [this repository](https://codeberg.org/OverLessArtem/ayugram-ebuild-gentoo) for installation manual.

### Void Linux
See [this repository](https://codeberg.org/OverLessArtem/ayugram-template-void) for installation manual.

### EPM

`epm play ayugram`

### Fedora

From [RPM Fusion](https://admin.rpmfusion.org/pkgdb/package/free/ayugram-desktop/) repository.

```bash
dnf install ayugram-desktop
```

### Any other Linux distro

Flatpak: https://github.com/0FL01/AyuGramDesktop-flatpak

Or follow the [official guide](https://github.com/AyuGram/AyuGramDesktop/blob/dev/docs/building-linux.md).

## Donation

Enjoy using **AyuGram**? Consider sending us a tip!

[Here's available methods.](https://docs.ayugram.one/donate/)

---

## 致谢

- 原项目：[AyuGram/AyuGramDesktop](https://github.com/AyuGram/AyuGramDesktop)
- Telegram Desktop：[telegramdesktop/tdesktop](https://github.com/telegramdesktop/tdesktop)
- 其他客户端：[Kotatogram](https://github.com/kotatogram/kotatogram-desktop)、[64Gram](https://github.com/TDesktop-x64/tdesktop)、[Forkgram](https://github.com/forkgram/tdesktop)
- 使用的库：[JSON for Modern C++](https://github.com/nlohmann/json)、[SQLite](https://github.com/sqlite/sqlite)、[sqlite_orm](https://github.com/fnc12/sqlite_orm)、[androidx sources](https://github.com/androidx/androidx)
- 图标：[Solar Icon Set](https://www.figma.com/community/file/1166831539721848736)
