// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/utils/ayu_mapper.h"

#include "apiwrap.h"
#include "api/api_text_entities.h"
#include "data/data_document.h"
#include "data/data_media_types.h"
#include "data/data_photo.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_components.h"
#include "main/main_session.h"
#include "mtproto/connection_abstract.h"
#include "mtproto/details/mtproto_dump_to_text.h"

namespace AyuMapper {

constexpr auto kMessageFlagUnread = 0x00000001;
constexpr auto kMessageFlagOut = 0x00000002;
constexpr auto kMessageFlagForwarded = 0x00000004;
constexpr auto kMessageFlagReply = 0x00000008;
constexpr auto kMessageFlagMention = 0x00000010;
constexpr auto kMessageFlagContentUnread = 0x00000020;
constexpr auto kMessageFlagHasMarkup = 0x00000040;
constexpr auto kMessageFlagHasEntities = 0x00000080;
constexpr auto kMessageFlagHasFromId = 0x00000100;
constexpr auto kMessageFlagHasMedia = 0x00000200;
constexpr auto kMessageFlagHasViews = 0x00000400;
constexpr auto kMessageFlagHasBotId = 0x00000800;
constexpr auto kMessageFlagIsSilent = 0x00001000;
constexpr auto kMessageFlagIsPost = 0x00004000;
constexpr auto kMessageFlagEdited = 0x00008000;
constexpr auto kMessageFlagHasPostAuthor = 0x00010000;
constexpr auto kMessageFlagIsGrouped = 0x00020000;
constexpr auto kMessageFlagFromScheduled = 0x00040000;
constexpr auto kMessageFlagHasReactions = 0x00100000;
constexpr auto kMessageFlagHideEdit = 0x00200000;
constexpr auto kMessageFlagRestricted = 0x00400000;
constexpr auto kMessageFlagHasReplies = 0x00800000;
constexpr auto kMessageFlagIsPinned = 0x01000000;
constexpr auto kMessageFlagHasTTL = 0x02000000;
constexpr auto kMessageFlagInvertMedia = 0x08000000;
constexpr auto kMessageFlagHasSavedPeer = 0x10000000;

template<typename MTPObject>
std::vector<char> serializeObject(MTPObject object) {
	mtpBuffer buffer;
	object.write(buffer);

	const auto from = reinterpret_cast<char*>(buffer.data());
	const auto end = from + buffer.size() * sizeof(mtpPrime);

	std::vector<char> entities(from, end);
	return entities;
}

template<typename MTPObject>
MTPObject deserializeObject(std::vector<char> serialized) {
	gsl::span<char> span(serialized.data(), serialized.size());

	auto from = reinterpret_cast<const mtpPrime*>(span.data());
	const auto end = from + span.size() / sizeof(mtpPrime);

	MTPObject data;
	if (!data.read(from, end)) {
		LOG(("AyuMapper: Failed to deserialize object"));
	}
	return data;
}

std::pair<std::string, std::vector<char>> serializeTextWithEntities(not_null<HistoryItem*> item) {
	if (item->emptyText()) {
		return std::make_pair("", std::vector<char>());
	}
	auto textWithEntities = item->originalText();


	std::vector<char> entities;
	if (!textWithEntities.entities.empty()) {
		const auto mtpEntities = Api::EntitiesToMTP(
			&item->history()->session(),
			textWithEntities.entities,
			Api::ConvertOption::WithLocal);

		entities = serializeObject(mtpEntities);
	}

	return std::make_pair(textWithEntities.text.toStdString(), entities);
}

MTPVector<MTPMessageEntity> deserializeTextWithEntities(std::vector<char> serialized) {
	return deserializeObject<MTPVector<MTPMessageEntity>>(serialized);
}

namespace {

[[nodiscard]] MTPPhoto RebuildPhoto(not_null<PhotoData*> photo) {
	const auto input = photo->mtpInput();
	return input.match([&](const MTPDinputPhoto &data) {
		auto sizes = QVector<MTPPhotoSize>();
		sizes.push_back(MTP_photoSize(
			MTP_string("m"),
			MTP_int(photo->width()),
			MTP_int(photo->height()),
			MTP_int(0)));
		return MTP_photo(
			MTP_flags(0),
			data.vid(),
			data.vaccess_hash(),
			MTP_bytes(data.vfile_reference().v),
			MTP_int(photo->date()),
			MTP_vector<MTPPhotoSize>(sizes),
			MTPVector<MTPVideoSize>(),
			MTP_int(photo->getDC()));
	}, [&](const auto &) {
		return MTP_photoEmpty(MTP_long(photo->id));
	});
}

[[nodiscard]] MTPDocument RebuildDocument(not_null<DocumentData*> document) {
	const auto input = document->mtpInput();
	return input.match([&](const MTPDinputDocument &data) {
		auto attributes = QVector<MTPDocumentAttribute>();
		if (!document->filename().isEmpty()) {
			attributes.push_back(MTP_documentAttributeFilename(
				MTP_string(document->filename())));
		}
		if (document->type == VideoDocument
			|| document->type == RoundVideoDocument
			|| document->isVideoFile()
			|| document->isVideoMessage()) {
			using Flag = MTPDdocumentAttributeVideo::Flag;
			auto flags = MTPDdocumentAttributeVideo::Flags(0);
			if (document->type == RoundVideoDocument
				|| document->isVideoMessage()) {
				flags |= Flag::f_round_message;
			}
			attributes.push_back(MTP_documentAttributeVideo(
				MTP_flags(flags),
				MTP_double(document->duration() / 1000.),
				MTP_int(document->dimensions.width()),
				MTP_int(document->dimensions.height()),
				MTPint(),
				MTPdouble(),
				MTPstring()));
		} else if (document->dimensions.width() > 0
			&& document->dimensions.height() > 0) {
			attributes.push_back(MTP_documentAttributeImageSize(
				MTP_int(document->dimensions.width()),
				MTP_int(document->dimensions.height())));
		}
		if (document->isAnimation()) {
			attributes.push_back(MTP_documentAttributeAnimated());
		}
		auto thumbs = QVector<MTPPhotoSize>();
		const auto thumb = document->thumbnailLocation();
		if (thumb.valid()) {
			thumbs.push_back(MTP_photoSize(
				MTP_string("s"),
				MTP_int(thumb.width()),
				MTP_int(thumb.height()),
				MTP_int(document->thumbnailByteSize())));
		}
		const auto flags = thumbs.isEmpty()
			? MTPDdocument::Flag()
			: MTPDdocument::Flag::f_thumbs;
		return MTP_document(
			MTP_flags(flags),
			data.vid(),
			data.vaccess_hash(),
			MTP_bytes(data.vfile_reference().v),
			MTP_int(document->date),
			MTP_string(document->mimeString()),
			MTP_long(document->size),
			MTP_vector<MTPPhotoSize>(thumbs),
			MTPVector<MTPVideoSize>(),
			MTP_int(document->getDC()),
			MTP_vector<MTPDocumentAttribute>(attributes));
	}, [](const MTPDinputDocumentEmpty &) {
		return MTPDocument();
	}, [](const auto &) {
		return MTPDocument();
	});
}

} // namespace

std::vector<char> serializeMessageMedia(not_null<HistoryItem*> item) {
	const auto media = item->media();
	if (!media) {
		return {};
	}
	auto result = MTPMessageMedia(MTP_messageMediaEmpty());
	if (const auto photo = media->photo()) {
		const auto rebuilt = RebuildPhoto(photo);
		if (rebuilt.type() != mtpc_photo) {
			return {};
		}
		using Flag = MTPDmessageMediaPhoto::Flag;
		result = MTP_messageMediaPhoto(
			MTP_flags(Flag::f_photo),
			rebuilt,
			MTP_int(0),
			MTPDocument());
	} else if (const auto document = media->document()) {
		const auto rebuilt = RebuildDocument(document);
		if (rebuilt.type() != mtpc_document) {
			return {};
		}
		using Flag = MTPDmessageMediaDocument::Flag;
		auto flags = Flag::f_document;
		if (document->isVideoFile() || document->isVideoMessage()) {
			flags |= Flag::f_video;
		}
		if (document->isVideoMessage()) {
			flags |= Flag::f_round;
		}
		if (document->isVoiceMessage()) {
			flags |= Flag::f_voice;
		}
		result = MTP_messageMediaDocument(
			MTP_flags(flags),
			rebuilt,
			MTPVector<MTPDocument>(),
			MTPPhoto(),
			MTPint(),
			MTP_int(0));
	}
	if (result.type() == mtpc_messageMediaEmpty) {
		return {};
	}
	return serializeObject(result);
}

MTPMessageMedia deserializeMessageMedia(std::vector<char> serialized) {
	if (serialized.empty()) {
		return MTP_messageMediaEmpty();
	}
	auto result = deserializeObject<MTPMessageMedia>(serialized);
	const auto supported = (result.type() == mtpc_messageMediaPhoto)
		|| (result.type() == mtpc_messageMediaDocument);
	return supported ? result : MTPMessageMedia(MTP_messageMediaEmpty());
}

int mapItemFlagsToMTPFlags(not_null<HistoryItem*> item) {
	int flags = 0;

	const auto thread = item->topic()
							? reinterpret_cast<Data::Thread*>(item->topic())
							: item->history();
	if (item->unread(thread)) {
		flags |= kMessageFlagUnread;
	}

	if (item->out()) {
		flags |= kMessageFlagOut;
	}

	if (item->Get<HistoryMessageForwarded>()) {
		flags |= kMessageFlagForwarded;
	}

	if (item->Get<HistoryMessageReply>()) {
		flags |= kMessageFlagReply;
	}

	if (item->mentionsMe()) {
		flags |= kMessageFlagMention;
	}

	if (item->hasUnreadMediaFlag()) {
		flags |= kMessageFlagContentUnread;
	}

	if (item->definesReplyKeyboard()) {
		flags |= kMessageFlagHasMarkup;
	}

	if (!item->originalText().entities.empty()) {
		flags |= kMessageFlagHasEntities;
	}

	if (item->displayFrom()) {
		// todo: maybe wrong
		flags |= kMessageFlagHasFromId;
	}

	if (item->media()) {
		flags |= kMessageFlagHasMedia;
	}

	if (item->hasViews()) {
		flags |= kMessageFlagHasViews;
	}

	if (item->viaBot()) {
		flags |= kMessageFlagHasBotId;
	}

	if (item->isSilent()) {
		flags |= kMessageFlagIsSilent;
	}

	if (item->isPost()) {
		flags |= kMessageFlagIsPost;
	}

	if (item->Get<HistoryMessageEdited>()) {
		flags |= kMessageFlagEdited;
	}

	if (item->Get<HistoryMessageSigned>()) {
		flags |= kMessageFlagHasPostAuthor;
	}

	if (item->groupId()) {
		flags |= kMessageFlagIsGrouped;
	}

	if (item->isScheduled()) {
		flags |= kMessageFlagFromScheduled;
	}

	if (!item->reactions().empty()) {
		flags |= kMessageFlagHasReactions;
	}

	if (item->hideEditedBadge()) {
		flags |= kMessageFlagHideEdit;
	}

	if (item->hasPossibleRestrictions()) {
		flags |= kMessageFlagRestricted;
	}

	if (item->repliesCount() > 0) {
		flags |= kMessageFlagHasReplies;
	}

	if (item->isPinned()) {
		flags |= kMessageFlagIsPinned;
	}

	if (item->ttlDestroyAt() > 0) {
		flags |= kMessageFlagHasTTL;
	}

	if (item->invertMedia()) {
		flags |= kMessageFlagInvertMedia;
	}

	if (item->savedFromSender()) {
		// todo: maybe wrong
		flags |= kMessageFlagHasSavedPeer;
	}

	return flags;
}

}
