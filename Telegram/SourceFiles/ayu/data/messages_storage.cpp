// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/data/messages_storage.h"

#include "ayu/data/ayu_database.h"
#include "ayu/utils/ayu_mapper.h"
#include "ayu/utils/telegram_helpers.h"
#include "ayu/ayu_settings.h"
#include "api/api_text_entities.h"
#include "base/unixtime.h"
#include "data/data_forum_topic.h"
#include "data/data_session.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_user.h"
#include "data/data_msg_id.h"
#include "data/data_peer_id.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_components.h"
#include "main/main_session.h"
#include "ui/text/text_utilities.h"

#include <map>
#include <set>

namespace AyuMessages {
namespace {

using ReinjectedKey = std::pair<ID, ID>;
std::map<ReinjectedKey, std::set<int>> ReinjectedIds;

} // namespace

template<typename DerivedMessage>
std::vector<AyuMessageBase> convertToBase(const std::vector<DerivedMessage> &messages) {
	std::vector<AyuMessageBase> based;
	based.reserve(messages.size());
	for (const auto &msg : messages) {
		based.push_back(static_cast<AyuMessageBase>(msg));
	}
	return based;
}

void map(not_null<HistoryItem*> item, AyuMessageBase &message) {
	const ID userId = item->history()->owner().session().userId().bare & PeerId::kChatTypeMask;

	message.userId = userId;
	message.dialogId = getDialogIdFromPeer(item->history()->peer);
	message.groupedId = item->groupId().raw();
	message.peerId = item->history()->peer->id.value & PeerId::kChatTypeMask;
	message.fromId = item->from()->id.value & PeerId::kChatTypeMask;
	if (item->topic()) {
		message.topicId = item->topicRootId().bare;
	} else {
		message.topicId = 0;
	}
	message.messageId = item->id.bare;
	message.date = item->date();
	message.flags = AyuMapper::mapItemFlagsToMTPFlags(item);

	if (const auto edited = item->Get<HistoryMessageEdited>()) {
		message.editDate = edited->date;
	} else {
		message.editDate = base::unixtime::now();
	}

	message.views = item->viewsCount();
	message.fwdFlags = 0;
	message.fwdFromId = 0;
	// message.fwdName
	message.fwdDate = 0;
	// message.fwdPostAuthor
	if (const auto msgsigned = item->Get<HistoryMessageSigned>()) {
		message.postAuthor = msgsigned->author.toStdString();
	}
	message.replyFlags = 0;
	message.replyMessageId = 0;
	message.replyPeerId = 0;
	message.replyTopId = 0;
	message.replyForumTopic = false;
	// message.replySerialized
	// message.replyMarkupSerialized
	message.entityCreateDate = base::unixtime::now();

	auto serializedText = AyuMapper::serializeTextWithEntities(item);
	message.text = serializedText.first;
	message.textEntities = serializedText.second;

	message.documentSerialized = AyuMapper::serializeMessageMedia(item);
	message.documentType = message.documentSerialized.empty() ? 0 : 1;
	message.mediaPath = "/";
	if (const auto media = item->media()) {
		if (const auto document = media->document()) {
			message.mimeType = document->mimeString().toStdString();
		}
	}
}

void addEditedMessage(not_null<HistoryItem *> item) {
	EditedMessage message;
	map(item, message);

	if (message.text.empty() && message.documentSerialized.empty()) {
		return;
	}

	AyuDatabase::addEditedMessage(message);
}

std::vector<AyuMessageBase> getEditedMessages(not_null<HistoryItem*> item, ID minId, ID maxId, int totalLimit) {
	const ID userId = item->history()->owner().session().userId().bare & PeerId::kChatTypeMask;
	const auto dialogId = getDialogIdFromPeer(item->history()->peer);
	const auto msgId = item->id.bare;

	return convertToBase(AyuDatabase::getEditedMessages(userId, dialogId, msgId, minId, maxId, totalLimit));
}

bool hasRevisions(not_null<HistoryItem*> item) {
	const ID userId = item->history()->owner().session().userId().bare & PeerId::kChatTypeMask;
	const auto dialogId = getDialogIdFromPeer(item->history()->peer);
	const auto msgId = item->id.bare;

	return AyuDatabase::hasRevisions(userId, dialogId, msgId);
}

void addDeletedMessage(not_null<HistoryItem*> item) {
	DeletedMessage message;
	map(item, message);

	if (message.text.empty() && message.documentSerialized.empty()) {
		return;
	}

	AyuDatabase::addDeletedMessage(message);
}

std::vector<AyuMessageBase>
getDeletedMessages(not_null<PeerData*> peer, ID topicId, ID minId, ID maxId, int totalLimit, const QString &searchQuery) {
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	return convertToBase(
		AyuDatabase::getDeletedMessages(userId, getDialogIdFromPeer(peer), topicId, minId, maxId, totalLimit, searchQuery.toStdString()));
}

bool hasDeletedMessages(not_null<PeerData*> peer, ID topicId) {
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	return AyuDatabase::hasDeletedMessages(userId, getDialogIdFromPeer(peer), topicId);
}

void removeDeletedMessage(not_null<HistoryItem*> item) {
	const auto peer = item->history()->peer;
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	AyuDatabase::removeDeletedMessage(userId, getDialogIdFromPeer(peer), item->id.bare);
}

void clearDeletedMessages(not_null<PeerData*> peer, ID topicId) {
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	const auto dialogId = getDialogIdFromPeer(peer);
	AyuDatabase::clearDeletedMessages(userId, dialogId, topicId);
	ReinjectedIds.erase({ userId, dialogId });
}

void reinjectDeletedMessages(not_null<History*> history) {
	const auto &settings = AyuSettings::getInstance();
	if (!settings.keepDeletedMessagesInChat()) {
		return;
	}

	const auto peer = history->peer;
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	const auto dialogId = getDialogIdFromPeer(peer);

	// A fresh History has no client-side items, so its registry must start empty.
	if (history->clientSideMessages().empty()) {
		ReinjectedIds.erase({ userId, dialogId });
	}

	auto messages = AyuDatabase::getDeletedMessages(userId, dialogId, 0, 0, 0, 5000, "");

	if (messages.empty()) {
		return;
	}

	auto &owner = history->owner();

	for (const auto &msg : messages) {
		const auto serverMsgId = MsgId(msg.messageId);
		if (const auto existing = owner.message(peer->id, serverMsgId)) {
			if (!existing->isDeleted()) {
				continue;
			}
		}

		PeerData *from = owner.userLoaded(UserId(msg.fromId));
		if (!from) {
			from = owner.channelLoaded(ChannelId(msg.fromId));
		}
		if (!from) {
			from = owner.chatLoaded(ChatId(msg.fromId));
		}

		// Reinjection runs on every loaded slice, so inject each stored row once.
		if (!ReinjectedIds[{ userId, dialogId }].insert(msg.messageId).second) {
			continue;
		}

		const auto text = QString::fromStdString(msg.text);
		const auto media = AyuMapper::deserializeMessageMedia(msg.documentSerialized);

		const auto localMsgId = owner.nextLocalMessageId();

		auto textAndEntities = TextWithEntities(text);
		const auto entities = AyuMapper::deserializeTextWithEntities(msg.textEntities);
		textAndEntities.entities = Api::EntitiesFromMTP(&peer->session(), entities.v);

		const ID currentUserId = peer->session().userId().bare & PeerId::kChatTypeMask;
		const bool isOutgoing = (msg.fromId == currentUserId);

		base::flags<MessageFlag> flags = MessageFlag::Local | MessageFlag::HistoryEntry | MessageFlag::ClientSideUnread;
		if (isOutgoing && !peer->isSelf()) {
			flags |= MessageFlag::Outgoing;
		}
		if (from) {
			flags |= MessageFlag::HasFromId;
		}
		if (!msg.postAuthor.empty()) {
			flags |= MessageFlag::HasPostAuthor;
		}

		const auto item = history->makeMessage({
			.id = localMsgId,
			.flags = flags,
			.from = from ? from->id : PeerId(0),
			.date = msg.date,
			.groupedId = msg.groupedId,
			.postAuthor = !msg.postAuthor.empty() ? QString::fromStdString(msg.postAuthor) : QString(),
		}, std::move(textAndEntities), media);

		item->setDeleted();

		history->registerClientSideMessage(item);
	}
}

}
