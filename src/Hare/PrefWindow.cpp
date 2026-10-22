/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "PrefWindow.h"

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <SeparatorView.h>
#include <TranslationDefs.h>
#include <TranslationKit.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>

#include "AEEncoder.h"
#include "AppDefs.h"
#include "CommandConstants.h"
#include "GUIStrings.h"
#include "Settings.h"
#include "tagkit/CoverArtImage.h"

// Fills menu with an item for each image format a Translation Kit
// translator can write (JPEG, PNG, ...), the one for currentMime marked, or
// JPEG if that one isn't there (any more). Always has at least JPEG, so
// there's something to choose even without translators.
static void
FillCoverFormatMenu(BMenu* menu, const BString& currentMime)
{
	std::vector<BString> mimeTypes;
	std::vector<BString> labels;

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	translator_id* ids = NULL;
	int32 count = 0;
	if (roster != NULL && roster->GetAllTranslators(&ids, &count) == B_OK) {
		for (int32 i = 0; i < count; i++) {
			const translation_format* formats = NULL;
			int32 formatCount = 0;
			if (roster->GetOutputFormats(ids[i], &formats, &formatCount)
					!= B_OK || formats == NULL) {
				continue;
			}
			for (int32 j = 0; j < formatCount; j++) {
				// Only the formats of bitmaps, and not the Be bitmap itself.
				if (formats[j].group != B_TRANSLATOR_BITMAP
						|| formats[j].type == B_TRANSLATOR_BITMAP) {
					continue;
				}
				BString mime(formats[j].MIME);
				BString extension = tagkit::image_extension_for_mime_type(
					mime.String());
				if (extension.IsEmpty())
					continue;

				bool known = false;
				for (size_t k = 0; k < mimeTypes.size(); k++) {
					if (mimeTypes[k] == mime)
						known = true;
				}
				if (known)
					continue;

				BString label(formats[j].name);
				label << " (." << extension << ")";
				mimeTypes.push_back(mime);
				labels.push_back(label);
			}
		}
		delete[] ids;
	}

	if (mimeTypes.empty()) {
		mimeTypes.push_back(BString("image/jpeg"));
		labels.push_back(BString("JPEG (.jpg)"));
	}

	bool haveCurrent = false;
	for (size_t i = 0; i < mimeTypes.size(); i++) {
		if (mimeTypes[i] == currentMime)
			haveCurrent = true;
	}
	BString marked = haveCurrent ? currentMime : BString("image/jpeg");

	for (size_t i = 0; i < mimeTypes.size(); i++) {
		BMessage* message = new BMessage(PREF_COVER_FORMAT);
		message->AddString("mime", mimeTypes[i]);
		BMenuItem* item = new BMenuItem(labels[i].String(), message);
		item->SetMarked(mimeTypes[i] == marked);
		menu->AddItem(item);
	}
	if (menu->FindMarked() == NULL && menu->ItemAt(0) != NULL)
		menu->ItemAt(0)->SetMarked(true);

	// What the menu shows as chosen is what's used, also when the format
	// saved earlier isn't available any more.
	BMenuItem* item = menu->FindMarked();
	const char* mime = NULL;
	if (item != NULL && item->Message() != NULL
			&& item->Message()->FindString("mime", &mime) == B_OK) {
		settings->SetCoverFileMimeType(mime);
	}
}


PrefWindow::PrefWindow()
	:
	BWindow(BRect(), PREFS, B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
				B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS |
				B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE)
{
	InitWindow();
	CenterOnScreen();	
}

PrefWindow::~PrefWindow()
{
}

void
PrefWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case SAVE_PREFS: {
				// The pattern belongs to whichever encoder is currently
				// selected - AEEncoder persists it to that encoder's own
				// settings file (keyed by encoder name), so switching
				// encoders never clobbers another encoder's pattern.
				AEEncoder* encoder = settings->Encoder();
				if (encoder) {
					encoder->SetPattern(fileNamePatternTextControl->Text());
				}
				settings->SaveSettings();
				be_app_messenger.SendMessage(new BMessage(FILE_NAME_PATTERN_CHANGED));
			}
			break;
		case REVERT_PREFS: {
				AEEncoder* encoder = settings->Encoder();
				if (encoder) {
					// GetPattern() falls back to (and persists) the
					// encoder's own default if its pattern is blank/unset.
					fileNamePatternTextControl->SetText(encoder->GetPattern());
				}
			}
			break;
		case ENCODER_PATTERN_PREFS: {
			AEEncoder* encoder = settings->Encoder();
			if (encoder) {
				fileNamePatternTextControl->SetText(encoder->GetDefaultPattern());		
			}
			}
			break;
		case PREF_OPTION_CHANGED: {
				int32 option = -1;
				int32 value = 0;
				if (message->FindInt32("option", &option) == B_OK
						&& message->FindInt32("be:value", &value) == B_OK
						&& option >= 0 && option < PREF_OPTION_COUNT) {
					settings->SetOption((pref_option)option,
						value == B_CONTROL_ON);
					settings->SaveSettings();
					UpdateOptionsEnabled();
				}
			}
			break;
		case PREF_COVER_FORMAT: {
				const char* mime = NULL;
				if (message->FindString("mime", &mime) == B_OK) {
					settings->SetCoverFileMimeType(mime);
					settings->SaveSettings();
				}
			}
			break;
		default:
			BWindow::MessageReceived(message);
	}
}


BCheckBox*
PrefWindow::AddOption(pref_option option, const char* label)
{
	BMessage* message = new BMessage(PREF_OPTION_CHANGED);
	message->AddInt32("option", option);
	BCheckBox* box = new BCheckBox("option", label, message);
	box->SetValue(settings->Option(option) ? B_CONTROL_ON : B_CONTROL_OFF);
	optionBoxes[option] = box;
	return box;
}


BBox*
PrefWindow::InitTaggingBox()
{
	BBox* box = new BBox("taggingBox");
	box->SetLabel(TAGGING_BOX_LABEL);

	BCheckBox* writeTags = AddOption(PREF_WRITE_TAGS, PREF_WRITE_TAGS_TXT);
	BCheckBox* embedCover = AddOption(PREF_EMBED_COVER_ART,
		PREF_EMBED_COVER_ART_TXT);
	BCheckBox* tagIds = AddOption(PREF_TAG_MUSICBRAINZ_IDS,
		PREF_TAG_MUSICBRAINZ_IDS_TXT);
	BCheckBox* writeAttributes = AddOption(PREF_WRITE_ATTRIBUTES,
		PREF_WRITE_ATTRIBUTES_TXT);
	BCheckBox* attributeIds = AddOption(PREF_ATTRIBUTE_MUSICBRAINZ_IDS,
		PREF_ATTRIBUTE_MUSICBRAINZ_IDS_TXT);
	BCheckBox* saveCover = AddOption(PREF_SAVE_COVER_FILE,
		PREF_SAVE_COVER_FILE_TXT);
	BCheckBox* useMusicBrainz = AddOption(PREF_USE_MUSICBRAINZ,
		PREF_USE_MUSICBRAINZ_TXT);
	BCheckBox* fillYear = AddOption(PREF_FILL_YEAR, PREF_FILL_YEAR_TXT);

	BPopUpMenu* formatMenu = new BPopUpMenu("coverFormat");
	FillCoverFormatMenu(formatMenu, BString(settings->CoverFileMimeType()));
	formatMenu->SetTargetForItems(this);
	coverFormatField = new BMenuField("coverFormatField",
		PREF_COVER_FORMAT_LABEL, formatMenu);

	// The options that depend on another are indented under it.
	const float indent = be_plain_font->Size() * 2;

	BLayoutBuilder::Group<>(box, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_BIG_INSETS,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(writeTags)
		.AddGroup(B_VERTICAL, B_USE_HALF_ITEM_SPACING)
			.SetInsets(indent, 0, 0, 0)
			.Add(embedCover)
			.Add(tagIds)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(writeAttributes)
		.AddGroup(B_VERTICAL, B_USE_HALF_ITEM_SPACING)
			.SetInsets(indent, 0, 0, 0)
			.Add(attributeIds)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(saveCover)
		.AddGroup(B_HORIZONTAL)
			.SetInsets(indent, 0, 0, 0)
			.Add(coverFormatField)
			.AddGlue()
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(useMusicBrainz)
		.AddGroup(B_VERTICAL, B_USE_HALF_ITEM_SPACING)
			.SetInsets(indent, 0, 0, 0)
			.Add(fillYear)
		.End()
	.End();

	UpdateOptionsEnabled();
	return box;
}


void
PrefWindow::UpdateOptionsEnabled()
{
	bool tags = settings->Option(PREF_WRITE_TAGS);
	optionBoxes[PREF_EMBED_COVER_ART]->SetEnabled(tags);
	optionBoxes[PREF_TAG_MUSICBRAINZ_IDS]->SetEnabled(tags);
	optionBoxes[PREF_ATTRIBUTE_MUSICBRAINZ_IDS]->SetEnabled(
		settings->Option(PREF_WRITE_ATTRIBUTES));
	coverFormatField->SetEnabled(settings->Option(PREF_SAVE_COVER_FILE));
	optionBoxes[PREF_FILL_YEAR]->SetEnabled(
		settings->Option(PREF_USE_MUSICBRAINZ));
}

void
PrefWindow::InitWindow()
{
	AEEncoder* encoder = settings->Encoder();

	BBox* fileNamePatternBox = new BBox("fileNamePatternBox");
	if (encoder) {
		BString boxLabel(FILE_NAME_PATTERN_BOX_LABEL_FOR);
		boxLabel << encoder->GetName();
		fileNamePatternBox->SetLabel(boxLabel.String());
	} else {
		fileNamePatternBox->SetLabel(FILE_NAME_PATTERN_BOX_LABEL);
	}

	artistStringView = new BStringView("artist", ARTIST_LABEL);
	yearStringView = new BStringView("year", YEAR_LABEL);
	commentStringView = new BStringView("comment", COMMENT_LABEL);
	albumStringView = new BStringView("album", ALBUM_LABEL);
	trackStringView = new BStringView("track", TRACK_LABEL);
	titleStringView = new BStringView("title", TITLE_LABEL);
	genreStringView = new BStringView("genre", GENRE_LABEL);

	const char* str = "";
	if (encoder) {
		// GetPattern() falls back to (and persists) the encoder's own
		// default if its pattern is blank/unset.
		str = encoder->GetPattern();
	} else {
		str = settings->PathPattern();
	}
	fileNamePatternTextControl = new BTextControl("fileNamePatternTextControl",
													B_EMPTY_STRING, str, NULL);

	applyButton = new BButton("applyButton", APPLY_BTN,
							  new BMessage(SAVE_PREFS));
	revertButton = new BButton("revertButton", REVERT_BTN,
							  new BMessage(REVERT_PREFS));
	encoderPatternButton = new BButton("encoderPatternButton", ENCODER_PATTERN_BTN,
							  new BMessage(ENCODER_PATTERN_PREFS));
	
	BLayoutBuilder::Group<>(fileNamePatternBox, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_BIG_INSETS, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGrid(1.0f, 0.0f)
			.Add(artistStringView, 0, 0)
			.Add(albumStringView, 1, 0)
			.Add(titleStringView, 2, 0)
			.Add(yearStringView, 0, 1)
			.Add(trackStringView, 1, 1)
			.Add(genreStringView, 2, 1)
			.Add(commentStringView, 0, 2)
		.End()
		.Add(fileNamePatternTextControl)
		// The buttons are in the box: they're for the pattern in it.
		.AddGroup(B_HORIZONTAL)
			.Add(revertButton, 0.0f)
			.Add(encoderPatternButton, 0.0f)
			.Add(applyButton, 1.0f)
		.End()
	.End();
	
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fileNamePatternBox, 0.0f)
		.Add(InitTaggingBox(), 0.0f)
	.End();				
}
