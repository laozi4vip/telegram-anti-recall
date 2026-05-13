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
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_components.h"
#include "main/main_session.h"

namespace AyuMessages {

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

	// todo: implement mapping
	message.mediaPath = "/";
	// message.hqThumbPath
	message.documentType = 0; // document type none
	// message.documentSerialized
	// message.thumbsSerialized
	// message.documentAttributesSerialized
	// message.mimeType
}

void addEditedMessage(not_null<HistoryItem *> item) {
	EditedMessage message;
	map(item, message);

	if (message.text.empty()) {
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

	if (message.text.empty()) {
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

void clearDeletedMessages(not_null<PeerData*> peer, ID topicId) {
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	AyuDatabase::clearDeletedMessages(userId, getDialogIdFromPeer(peer), topicId);
}

void reinjectDeletedMessages(not_null<History*> history) {
	const auto &settings = AyuSettings::getInstance();
	if (!settings.keepDeletedMessagesInChat()) {
		return;
	}

	const auto peer = history->peer;
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	const auto dialogId = getDialogIdFromPeer(peer);

	// Get all deleted messages for this dialog from the database
	auto messages = AyuDatabase::getDeletedMessages(userId, dialogId, 0, 0, 0, 5000, "");

	if (messages.empty()) {
		return;
	}

	auto &owner = history->owner();

	for (const auto &msg : messages) {
		// Skip messages that still exist in the history as normal (non-deleted) items.
		const auto serverMsgId = MsgId(msg.messageId);
		if (const auto existing = owner.message(peer->id, serverMsgId)) {
			if (!existing->isDeleted()) {
				// Server still has this message, skip injection
				continue;
			}
		}

		PeerData *from = owner.userLoaded(msg.fromId);
		if (!from) {
			from = owner.channelLoaded(msg.fromId);
		}
		if (!from) {
			from = reinterpret_cast<PeerData*>(owner.chatLoaded(msg.fromId));
		}

		// Use a client-side message ID so this is treated as a local message
		const auto localMsgId = owner.nextLocalMessageId();

		const auto text = QString::fromStdString(msg.text);

		// Dedup: skip if a client-side deleted message with same text and date
		// already exists (prevents duplicates when addOlderSlice is called multiple times)
		bool alreadyExists = false;
		for (const auto &existingItem : history->clientSideMessages()) {
			if (existingItem->isDeleted()
				&& existingItem->date() == msg.date
				&& existingItem->text().text == text) {
				alreadyExists = true;
				break;
			}
		}
		if (alreadyExists) {
			continue;
		}

		auto textAndEntities = Ui::Text::WithEntities(text);
		const auto entities = AyuMapper::deserializeTextWithEntities(msg.textEntities);
		textAndEntities.entities = Api::EntitiesFromMTP(&peer->session(), entities.v);

		base::flags<MessageFlag> flags = MessageFlag::Local | MessageFlag::HistoryEntry | MessageFlag::ClientSideUnread;
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
			.postAuthor = !msg.postAuthor.empty() ? QString::fromStdString(msg.postAuthor) : QString(),
		}, std::move(textAndEntities), MTP_messageMediaEmpty());

		// Mark as deleted so it renders with the deleted mark / semi-transparent style
		item->setDeleted();

		// Register as a client-side message so it survives properly in the history
		history->registerClientSideMessage(item);
	}
}

}
