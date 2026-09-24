#include "AppView.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Button.h>
#include <Debug.h>
#include <Font.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <fs_info.h>
#include <image.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <Message.h>
#include <Node.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <ObjectList.h>
#include <OS.h>
#include <Path.h>
#include <Rect.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <SplitView.h>
#include <StatusBar.h>
#include <StringView.h>
#include <String.h>
#include <TextControl.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <ColumnListView.h>
#include <ColumnTypes.h>

#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>
#include <tstringlist.h>

#include "AEEncoder.h"
#include "AudioAttributes.h"
#include "GenreList.h"
#include "MusicBrainzAttributes.h"

#include "AppDefs.h"
#include "AppWindow.h"
#include "CommandConstants.h"
#include "CheckMark.h"
#include "CoverArtView.h"
#include "EditorView.h"
#include "EncoderListView.h"
#include "GUIStrings.h"
#include "PrefWindow.h"
#include "RefRow.h"
#include "Settings.h"
#include "StatusBarFilter.h"

#define SLASH "/"
#define ALT_SLASH "∕"
#define QUESTIONMARK "?"
#define ALT_QUESTIONMARK "❓"
#define BACKSLASH "\\"
#define ALT_BACKSLASH "〵"
#define QUOTE "\""
#define ALT_QUOTE "″"
#define ASTERISK "*"
#define ALT_ASTERISK "✱"
#define GREATERTHAN ">"
#define ALT_GREATERTHAN "〉"
#define LESSTHAN "<"
#define ALT_LESSTHAN "〈"
#define COLON ":"
#define ALT_COLON "∶"
#define PIPE "|"
#define ALT_PIPE "⏐"
#define DASH "-"
#define UNDERSCORE "_"


AppView::AppView()
	:
	BView("AppView", B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP),
	coverArtImageData(NULL),
	coverArtImageSize(0),
	fLastCheckedFreeBytes(-1),
	mbTrackMetadata(NULL)
{
	PRINT(("AppView::AppView(BRect)\n"));

}

AppView::~AppView()
{
	PRINT(("AppView::~AppView()\n"));

	stop_watching(this);

	delete[] (unsigned char*)coverArtImageData;
	delete mbTrackMetadata;
}

void
AppView::InitView()
{
	PRINT(("AppView::InitView()\n"));

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	editorView = new EditorView();
	editorScrollView = new BScrollView("editorScrollView", editorView, 
										B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS,
										false, true, B_NO_BORDER);
	editorScrollView->SetExplicitMinSize(BSize(0, 0));
	editorScrollView->ScrollBar(B_VERTICAL)->SetRange(0, 250);
	editorScrollView->ScrollBar(B_VERTICAL)->SetProportion(0.5);

	editorBoxView = new BBox("editorBoxView");
	editorBoxView->SetLabel(EDITOR_LABEL);
	editorBoxView->SetExplicitMinSize(BSize(0, 125));
	editorBoxView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	coverArtView = new CoverArtView();

	coverArtBoxView = new BBox("coverArtBoxView");
	coverArtBoxView->SetExplicitMinSize(BSize(100, 125));
	coverArtBoxView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	encodeButton = new BButton("encodeButton", ENCODE_BTN,
							   new BMessage(ENCODE_MSG));
	cancelButton = new BButton("cancelButton", CANCEL_BTN,
							   new BMessage(CANCEL_MSG));

	listView = new EncoderListView();
	listView->SetExplicitMinSize(BSize(0, 125));
	listView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	BString remaining(STATUS_TRAILING_LABEL);
	remaining << 0;
	statusBar = new BStatusBar("statusBar", STATUS_LABEL,
							   remaining.String());
	statusBar->AddFilter(new StatusBarFilter());
	
	BLayoutBuilder::Group<>(editorBoxView, B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_BIG_INSETS,
					B_USE_DEFAULT_SPACING, B_USE_BIG_INSETS)
		.Add(editorScrollView, 0.0f)
	.End();

	BLayoutBuilder::Group<>(coverArtBoxView, B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
					B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(coverArtView, 0.0f)
	.End();

	// Details editor and cover art side by side, with a vertical splitter
	// between them, sitting above the horizontal splitter that separates
	// this row from the track list below. Both BSplitViews are kept as
	// members so SaveLayout()/RestoreLayout() can read back and reapply
	// their current proportions.
	topSplitView = new BSplitView(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING);
	topSplitView->AddChild(editorBoxView, 2.0f);
	topSplitView->AddChild(coverArtBoxView, 1.0f);
	topSplitView->SetCollapsible(0, false);
	topSplitView->SetCollapsible(1, false);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS, B_USE_WINDOW_INSETS,
						B_USE_WINDOW_INSETS, B_USE_WINDOW_INSETS)
		.AddSplit(B_VERTICAL, B_USE_HALF_ITEM_SPACING)
			.GetSplitView(&mainSplitView)
			.Add(topSplitView, 0.0f)
			.Add(listView, 0.0f)
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(statusBar, 0.0f)
			.AddGroup(B_VERTICAL)
				.AddStrut(B_USE_HALF_ITEM_SPACING)
				.AddGroup(B_HORIZONTAL)
					.Add(cancelButton, 1.0f)
					.Add(encodeButton, 1.0f)
				.End()
			.End()
		.End()
	.End();
}

void
AppView::AttachedToWindow()
{
	PRINT(("AppView::AttachedToWindow()\n"));

	BView::AttachedToWindow();

	InitView();

	encodeButton->SetTarget(this);
	cancelButton->SetTarget(this);
}

void
AppView::MessageReceived(BMessage* message)
{
	//PRINT(("AppView::MessageReceived(BMessage*)\n"));
	switch (message->what) {
		case VIEW_SHORTCUT: {
				int32 view;
				message->FindInt32("view", &view);
				PRINT(("View Shortcut: %d\n", view));
				switch (view) {
					case 0:
						editorView->MakeFocus(true);
						break;
					case 1:
						listView->MakeFocus(true);
						break;
				}
			}
			break;
		case ENCODE_MSG: {
				Encode();
			}
			break;
		case CANCEL_MSG: {
				Cancel();
			}
			break;
		case SAVE_LAYOUT_MSG: {
				SaveLayout();
			}
			break;
		case COVER_ART_FOUND: {
				// Sent by MusicBrainzLookup (on its own thread) once it's
				// found - or failed to find - cover art for the disc most
				// recently loaded. Display it right away; the original
				// compressed bytes are kept around so EncodeThread() can
				// write them out alongside the encoded files later.
				BBitmap* bitmap = NULL;
				message->FindPointer("bitmap", (void**)&bitmap);
				if (bitmap) {
					coverArtView->SetCoverArt(bitmap);
				}

				const void* data;
				ssize_t size;
				if (message->FindData("imageData", B_RAW_TYPE, &data, &size)
						== B_OK) {
					delete[] (unsigned char*)coverArtImageData;
					coverArtImageData = new unsigned char[size];
					memcpy(coverArtImageData, data, size);
					coverArtImageSize = (size_t)size;

					const unsigned char* bytes = (const unsigned char*)data;
					if ((size >= 8) && (bytes[0] == 0x89) && (bytes[1] == 'P')
							&& (bytes[2] == 'N') && (bytes[3] == 'G')) {
						coverArtImageExt = "png";
					} else if ((size >= 3) && (bytes[0] == 0xFF)
							&& (bytes[1] == 0xD8) && (bytes[2] == 0xFF)) {
						coverArtImageExt = "jpg";
					} else if ((size >= 4) && (bytes[0] == 'G')
							&& (bytes[1] == 'I') && (bytes[2] == 'F')) {
						coverArtImageExt = "gif";
					} else {
						// The Cover Art Archive serves mostly JPEG/PNG - a
						// reasonable default for anything else unrecognized.
						coverArtImageExt = "jpg";
					}
				}
			}
			break;
		case METADATA_FOUND: {
				// Sent by MusicBrainzLookup (on its own thread) alongside
				// (or instead of - the two are independent) COVER_ART_FOUND,
				// once it's found release/track metadata for the disc most
				// recently loaded. Just stored for now; EncodeThread() reads
				// it back out once each track finishes encoding.
				const char* str;
				if (message->FindString("discId", &str) == B_OK) {
					mbDiscId = str;
				}
				if (message->FindString("releaseId", &str) == B_OK) {
					mbReleaseId = str;
				}
				if (message->FindString("releaseGroupId", &str) == B_OK) {
					mbReleaseGroupId = str;
				}
				if (message->FindString("artistId", &str) == B_OK) {
					mbArtistId = str;
				}

				delete mbTrackMetadata;
				mbTrackMetadata = new BObjectList<TrackMBMetadata, true>(20);

				int32 trackNumber;
				for (int32 i = 0; message->FindInt32("trackNumber", i,
						&trackNumber) == B_OK; i++) {
					const char* trackTitle = "";
					const char* trackArtist = "";
					const char* recordingId = "";
					message->FindString("trackTitle", i, &trackTitle);
					message->FindString("trackArtist", i, &trackArtist);
					message->FindString("recordingId", i, &recordingId);

					TrackMBMetadata* track = new TrackMBMetadata();
					track->number = trackNumber;
					track->title = trackTitle;
					track->artist = trackArtist;
					track->recordingId = recordingId;
					mbTrackMetadata->AddItem(track);
				}
			}
			break;
		case SELECT_ALL_MSG:
			if (listView->CountRows() > 0) {
				for (int index = 0; index < listView->CountRows(); index++){
					listView->AddToSelection(listView->RowAt(index));
				}
			}
			break;
		case DESELECT_ALL_MSG: {
				listView->DeselectAll();
 			}
			break;
		case REMOVE_MSG: {
				int32 device;
				if ((message->FindInt32("device", &device) == B_OK)) {
					RemoveDeviceItemsFromList(device);
				} else {
					thread_id thread = spawn_thread(AppView::RemoveItemsFromList,
													"RemoveItems", B_NORMAL_PRIORITY, (void*)this);
					resume_thread(thread);
				}
			}
			break;
		case PREFS_MSG: {
				PrefWindow* prefWin = new PrefWindow();
				prefWin->Show();
			}
			break;
		case LIST_SELECTION_MSG: {
				editorView->ListSelectionChanged(message);
			}
			break;
		case APPLY_ATTRIBUTE_CHANGES: {
				ApplyAttributeChanges(message);
			}
			break;
		case ENCODER_CHANGED:
		case FILE_NAME_PATTERN_CHANGED: {
				// Also fired when the user picks a different encoder from
				// the Encoder menu - refresh every row's output filename
				// against whichever encoder's pattern (and file extension)
				// is now selected.
				int32 numRows = listView->CountRows();
				for (int i = 0; i < numRows; i++) {
					BRefRow* row =
						(BRefRow*) listView->RowAt(i);
					SetSaveAsColumn(row);
					listView->InvalidateRow(row);
				}
			}
			break;
		case B_REFS_RECEIVED:
		case B_SIMPLE_DATA: {
				BList* args = new BList();
				args->AddItem(this);
				args->AddItem(new BMessage(*message));
				thread_id thread = spawn_thread(AppView::RefsRecievedWrapper,
												"RefsReceived", B_NORMAL_PRIORITY,
												(void*)args);
				resume_thread(thread);
			}
			break;
		case COLUMN_SET_SHOWN: {
				int32 column;
				bool show;
				message->FindInt32("column", &column);
				message->FindBool("show", &show);
				listView->ColumnAt(column)->SetVisible(show);
			}
			break;
		case B_NODE_MONITOR: {
				int32 opcode;
				if (message->FindInt32("opcode", &opcode) != B_OK) {
					return;
				}

				switch (opcode) {
					case B_ENTRY_MOVED:
						UpdateItem(message);
						break;
					case B_ENTRY_REMOVED: {
							node_ref nref;

							if (message->FindInt64("node", &nref.node) != B_OK) {
								break;
							}
							if (message->FindInt32("device", &nref.device) != B_OK) {
								break;
							}
							RemoveNodeFromList(&nref);
						}
						break;
				}
			}
			break;
		default:
			BView::MessageReceived(message);
	}
}

int32
AppView::RefsRecievedWrapper(void* args)
{
	PRINT(("AppView::RefsReceivedWrapper(void*)\n"));

	BList* params = (BList*)args;
	AppView* view = (AppView*)params->ItemAt(0);
	BMessage* message = (BMessage*)params->ItemAt(1);
	view->RefsReceived(message);
	delete message;
	delete params;
	return B_OK;
}

void
AppView::RefsReceived(BMessage* message)
{
	PRINT(("AppView::RefsReceived(BMessage*)\n"));

	BRefRow* row;

	entry_ref tmp_ref;
	int numRefs = 0;
	while (message->FindRef("refs", numRefs, &tmp_ref) == B_NO_ERROR) {
		BNode node(&tmp_ref);
		if (node.IsFile()) {
			BNodeInfo info(&node);
			char type[B_MIME_TYPE_LENGTH+1];
			info.GetType(type);
			if ((strncmp(type, "audio/", 6) == 0)
					&& (strcmp(type, CDDA_MIME_TYPE) != 0)) {
				bool found = false;
				for (int i = 0; i < listView->CountRows(); i++) {
					BRefRow* row = (BRefRow*)listView->RowAt(i);
					entry_ref* ref = row->EntryRef();
					if (*ref == tmp_ref) {
						found = true;
						break;
					}
				}
				if (!found) {
					if (LockLooper()) {
						node_ref nref;
						if (node.GetNodeRef(&nref) == B_OK) {
							row = new BRefRow(&tmp_ref, &nref);
							watch_node(&nref, B_WATCH_NAME, this);
						} else {
							row = new BRefRow(&tmp_ref);
						}
						InitializeColumn(row);
						listView->AddRow(row);
						UnlockLooper();
					}
				}
			}
		} else if (node.IsDirectory()) {
			BDirectory dir(&tmp_ref);
			if (dir.InitCheck() == B_OK) {
				BMessage refsMessage(B_REFS_RECEIVED);
				entry_ref ref;
				while (dir.GetNextRef(&ref) != B_ENTRY_NOT_FOUND) {
					refsMessage.AddRef("refs", &ref);
				}
				RefsReceived(&refsMessage);
			}
		} else if (node.IsSymLink()) {
			BEntry entry(&tmp_ref, true);
			if ((entry.InitCheck() == B_OK) && (entry.GetRef(&tmp_ref) == B_OK)) {
				BMessage refsMessage(B_REFS_RECEIVED);
				refsMessage.AddRef("refs", &tmp_ref);
				RefsReceived(&refsMessage);
			}
		}
		numRefs++;
	}
}

void
AppView::InitializeColumn(BRefRow* row)
{
	PRINT(("AppView::InitializeColumn(BRefRow*)\n"));
	
	entry_ref* ref = row->EntryRef();

	row->SetField(new BStringField(ref->name), FILE_COLUMN_INDEX);

	// Fall back to a title derived from the file name for every row by
	// default; anything more specific found below (a CD volume's "Artist
	// - Album" name, real BFS attributes) overrides it. Plenty of WAV
	// files never carry real tag data of their own - CD tracks off a
	// disc without CD-Text/CDDB, or files dragged in from a folder that
	// was copied without preserving BFS attributes (a plain filesystem
	// copy generally doesn't preserve them, especially across a non-BFS
	// volume) - and a name derived from the file beats a blank title.
	BString defaultTitle(ref->name);
	int32 extensionIndex = defaultTitle.FindLast(".");
	if (extensionIndex >= 0) {
		defaultTitle.Truncate(extensionIndex);
	}

	// Haiku's cdda driver (and most rippers) name each track's file with
	// a leading track number - "05 Mr. Brownstone.wav" - so strip a
	// leading 1-3 digit number plus whatever run of spaces/"."/"-"/"_"
	// separates it from the rest ("05 ", "05. ", "05 - ", "05-", ...)
	// before using what's left as the title. Only strips when there's
	// still something left afterward, so a title that's *only* digits
	// (track number and nothing else) is left alone.
	int32 titleStart = 0;
	while (titleStart < defaultTitle.Length()
			&& isdigit((unsigned char) defaultTitle[titleStart])) {
		titleStart++;
	}
	if (titleStart > 0 && titleStart <= 3) {
		int32 separatorStart = titleStart;
		while (titleStart < defaultTitle.Length()
				&& strchr(" .-_", defaultTitle[titleStart]) != NULL) {
			titleStart++;
		}
		if (titleStart > separatorStart && titleStart < defaultTitle.Length()) {
			defaultTitle.Remove(0, titleStart);
		}
	}

	row->SetField(new BStringField(defaultTitle.String()), TITLE_COLUMN_INDEX);

	BVolume volume(ref->device);
	fs_info fsinfo;
	fs_stat_dev(volume.Device(), &fsinfo);
	if (strcmp(fsinfo.fsh_name, "cdda") == 0) {
		char volume_name[B_FILE_NAME_LENGTH];
		volume.GetName(volume_name);

		if (strcmp(volume_name, "Audio CD") != 0) {
			BString artist(volume_name);
			int index = artist.FindFirst(" - ");
			if (index >= 0) {
				artist.Truncate(index);
				BString album(volume_name);
				index += 3;
				album.Remove(index, album.Length() - index);
				artist.Trim();
				album.Trim();
				row->SetField(new BStringField(artist.String()), ARTIST_COLUMN_INDEX);
				row->SetField(new BStringField(album.String()), ALBUM_COLUMN_INDEX);
			}
		}
	}

	if (volume.KnowsAttr()) {
		PRINT(("Volume knows attributes.\n"));

		BFile file(ref, B_READ_ONLY);
		AudioAttributes attributes(&file);
		PRINT_OBJECT(attributes);

		const char* artist = attributes.Artist();
		if (artist) {
			row->SetField(new BStringField(artist), ARTIST_COLUMN_INDEX);
		}

		const char* album = attributes.Album();
		if (album) {
			row->SetField(new BStringField(album), ALBUM_COLUMN_INDEX);
		}

		const char* title = attributes.Title();
		if (title) {
			row->SetField(new BStringField(title), TITLE_COLUMN_INDEX);
		}

		const char* year = attributes.Year();
		if (year) {
			row->SetField(new BStringField(year), YEAR_COLUMN_INDEX);
		}

		const char* comment = attributes.Comment();
		if (comment) {
			row->SetField(new BStringField(comment), COMMENT_COLUMN_INDEX);
		}

		const char* track = attributes.Track();
		if (track) {
			row->SetField(new BStringField(track), TRACK_COLUMN_INDEX);
		}

		const char* genre = attributes.Genre();
		if (genre) {
			row->SetField(new BStringField(genre), GENRE_COLUMN_INDEX);
		}
	}

	SetSaveAsColumn(row);
}

void
AppView::SetSaveAsColumn(BRefRow* row)
{
	PRINT(("AppView::SetSaveAsColumn(BRefRow*)\n"));

	BStringField* tmpField;
	BString artist;
	BString album;
	BString title;
	BString year;
	BString comment;
	BString track;
	BString genre;

	tmpField = (BStringField*)row->GetField(ARTIST_COLUMN_INDEX);
	if (tmpField != NULL) artist = tmpField->String();
	if (artist == NULL) artist = "";
	tmpField = (BStringField*)row->GetField(ALBUM_COLUMN_INDEX);
	if (tmpField != NULL) album = tmpField->String();
	if (album == NULL) album = "";
	tmpField = (BStringField*)row->GetField(TITLE_COLUMN_INDEX);
	if (tmpField != NULL) title = tmpField->String();
	if (title == NULL) title = "";
	tmpField = (BStringField*)row->GetField(YEAR_COLUMN_INDEX);
	if (tmpField != NULL) year = tmpField->String();
	if (year == NULL) year = "";
	tmpField = (BStringField*)row->GetField(COMMENT_COLUMN_INDEX);
	if (tmpField != NULL) comment = tmpField->String();
	if (comment == NULL) comment = "";
	tmpField = (BStringField*)row->GetField(TRACK_COLUMN_INDEX);
	if (tmpField != NULL) track = tmpField->String();
	if (track == NULL) track = "";
	tmpField = (BStringField*)row->GetField(GENRE_COLUMN_INDEX);
	if (tmpField != NULL) genre = tmpField->String();
	if (genre == NULL) genre = "";

	int32 charswapmode = 2;
	artist = ReplaceInvalidFileChars(artist, charswapmode);
	album = ReplaceInvalidFileChars(album, charswapmode);
	title = ReplaceInvalidFileChars(title, charswapmode);
	year = ReplaceInvalidFileChars(year, charswapmode);
	comment = ReplaceInvalidFileChars(comment, charswapmode);
	track = ReplaceInvalidFileChars(track, charswapmode);
	genre = ReplaceInvalidFileChars(genre, charswapmode);

	AEEncoder* encoder = settings->Encoder();
	if (encoder) {
		BString fileNamePattern = encoder->GetPattern();
		fileNamePattern.ReplaceAll("%a", artist);
		fileNamePattern.ReplaceAll("%n", album);
		fileNamePattern.ReplaceAll("%t", title);
		fileNamePattern.ReplaceAll("%y", year);
		fileNamePattern.ReplaceAll("%c", comment);
		fileNamePattern.ReplaceAll("%k", track);
		fileNamePattern.ReplaceAll("%g", genre);
		row->SetField(new BStringField(fileNamePattern.String()), SAVE_AS_COLUMN_INDEX);
	}
}

void
AppView::ApplyAttributeChanges(BMessage* message)
{
	PRINT(("AppView::ApplyAttributeChanges(BMessage*)\n"));

	type_code index_type;
	int32 numSelected;
	message->GetInfo("index", &index_type, &numSelected);

	int32 index;
	BRefRow* row;
	for (int i = 0; i < numSelected; i++) {
		if (message->FindInt32("index", i, &index) != B_OK) {
			return;
		}

		row = (BRefRow*)listView->RowAt(index);
		if (!row) {
			return;
		}

		BString tmpString;

		if (message->FindString("artist", &tmpString) == B_OK) {
			row->SetField(new BStringField(tmpString.String()), ARTIST_COLUMN_INDEX);
		}

		if (message->FindString("album", &tmpString) == B_OK) {
			row->SetField(new BStringField(tmpString.String()), ALBUM_COLUMN_INDEX);
		}

		if (message->FindString("title", &tmpString) == B_OK) {
			row->SetField(new BStringField(tmpString.String()), TITLE_COLUMN_INDEX);
		}

		if (message->FindString("year", &tmpString) == B_OK) {
			row->SetField(new BStringField(tmpString.String()), YEAR_COLUMN_INDEX);
		}

		if (message->FindString("comment", &tmpString) == B_OK) {
			row->SetField(new BStringField(tmpString.String()), COMMENT_COLUMN_INDEX);
		}

		if (message->FindString("track", &tmpString) == B_OK) {
			if (tmpString.CountChars() == 1) {
				tmpString.Prepend("0");
			}
			row->SetField(new BStringField(tmpString.String()), TRACK_COLUMN_INDEX);
		}

		if (message->FindString("genre", &tmpString) == B_OK) {
			row->SetField(new BStringField(tmpString.String()), GENRE_COLUMN_INDEX);
		}

		SetSaveAsColumn(row);
		listView->InvalidateRow(row);
	}
}

void
AppView::RemoveNodeFromList(node_ref* ref)
{
	BRefRow* row;
	node_ref* rowRef;
	int32 numRows = listView->CountRows();
	for (int i = numRows - 1; i >= 0; i--) {
		row = (BRefRow*)listView->RowAt(i);
		if (row) {
			rowRef = row->NodeRef();
			if (*ref == *rowRef) {
				if (settings->IsEncoding()) {
					(new BAlert(0, REMOVED_TXT, OK))->Go(0);
					Cancel();
				}
				watch_node(rowRef, B_STOP_WATCHING, this);
				listView->Deselect(listView->RowAt(i));
				listView->RemoveRow(listView->RowAt(i));
				listView->Invalidate();
				delete row;
			}
		}
	}
}

void
AppView::RemoveDeviceItemsFromList(int32 device)
{
	PRINT(("AppView::RemoveDeviceItemsFromList(int32)\n"));

	bool deleted = false;
	BRefRow* row;
	entry_ref* ref;
	int32 numRows = listView->CountRows();
	for (int i = numRows - 1; i >= 0; i--) {
		row = (BRefRow*)listView->RowAt(i);
		if (row) {
			ref = row->EntryRef();
			if (device == ref->device) {
				deleted = true;
				watch_node(row->NodeRef(), B_STOP_WATCHING, this);
				listView->Deselect(listView->RowAt(i));
				listView->RemoveRow(listView->RowAt(i));
				listView->Invalidate();
				delete row;
			}
		}
	}

	if (deleted && settings->IsEncoding()) {
		(new BAlert(0, REMOVED_TXT, OK))->Go(0);
		Cancel();
	}
}

int32
AppView::RemoveItemsFromList(void* args)
{
	PRINT(("AppView::RemoveItemsFromList(void*)\n"));

	AppView* view = (AppView*)args;
	BRefRow* row;
	int32 max = view->listView->CountRows();
	for (int i = max - 1; i >= 0; i--) {
		if (view->listView->RowAt(i)->IsSelected()) {
			PRINT(("%03d\n", i));
			if (view->LockLooper()) {
				row = (BRefRow*)view->listView->RowAt(i);
				view->listView->Deselect(row);
				view->listView->RemoveRow(row);
				watch_node(row->NodeRef(), B_STOP_WATCHING, view);
				delete row;
				view->listView->Invalidate();
				view->UnlockLooper();
			}
		}
	}

	return B_OK;
}

int32
AppView::UpdateItem(BMessage* message)
{
	PRINT(("AppView::UpdateItem(BMessage*)\n"));

	ino_t node;
	ino_t dir;
	dev_t dev;
	const char* name;

	if (message->FindInt64("node", &node) != B_OK) {
		return B_ERROR;
	}

	if (message->FindInt64("to directory", &dir) != B_OK) {
		return B_ERROR;
	}

	if (message->FindInt32("device", &dev) != B_OK) {
		return B_ERROR;
	}

	if (message->FindString("name", &name) != B_OK) {
		return B_ERROR;
	}

	PRINT(("device = %d\n", dev));

	node_ref nref;
	entry_ref eref;

	nref.device = dev;
	nref.node = node;

	eref.device = dev;
	eref.directory = dir;
	eref.set_name(name);

	bool doVolume = false;
	if (dev == 1) {
		BVolumeRoster roster;
		BVolume volume;
		char volName[B_PATH_NAME_LENGTH];
		roster.Rewind();
		while (roster.GetNextVolume(&volume) == B_OK) {
			if (volume.InitCheck() != B_OK) {
				return B_ERROR;
			}
			volume.GetName(volName);
			if (strcmp(volName, name) == 0) {
				doVolume = true;
				dev = volume.Device();
				break;
			}
		}
	}

	BRefRow* row;
	int32 max = listView->CountRows();
	for (int i = 0; i < max; i++) {
		row = (BRefRow*)listView->RowAt(i);
		if (row) {
			PRINT(("item's device = %d\n", row->NodeRef()->device));
			if (row->NodeRef()->node == node) {
				row->SetNodeRef(&nref);
				row->SetEntryRef(&eref);
				if (LockLooper()) {
					InitializeColumn(row);
					listView->Invalidate();
					UnlockLooper();
				}
			} else if (doVolume && (row->NodeRef()->device == dev)) {
				if (LockLooper()) {
					InitializeColumn(row);
					listView->Invalidate();
					UnlockLooper();
				}
			}
		}
	}


	return B_OK;
}

namespace {

// Encoders read their input file sequentially, one buffer at a time, which
// is fine on a local disk but slow when the ref points at a track on a
// live "cdda" (audio CD) volume: every read there is real optical-drive
// I/O, often well under 1x realtime and hurt further by an encoder that
// only reads in small chunks. Copying the whole track to local disk first
// with a single, large, sequential read/write loop and encoding from that
// copy instead is dramatically faster in practice, and it's what CD
// rippers generally do rather than encoding directly off the disc.
//
// TempCDCopy is a small RAII helper: constructing it does the copy (a
// no-op, falling back to the original path, for anything not on a cdda
// volume) and its destructor removes the temp copy, so a track's temp
// file is cleaned up automatically at the end of its loop iteration no
// matter which of EncodeThread()'s many early-return paths gets taken.
class TempCDCopy {
public:
	// progressMessenger, when given, gets sent B_UPDATE_STATUS_BAR
	// messages (the same protocol the encoders already use on this same
	// messenger/statusBar) as the copy proceeds, so a slow optical drive
	// shows real progress during the rip instead of sitting at 0% until
	// encoding starts.
	TempCDCopy(const entry_ref* ref, const BPath& originalPath,
		BMessenger* progressMessenger = NULL)
		:
		fPath(originalPath),
		fIsTemp(false),
		fCanceled(false)
	{
		BVolume volume(ref->device);
		fs_info info;
		if (fs_stat_dev(volume.Device(), &info) != B_OK
				|| strcmp(info.fsh_name, "cdda") != 0) {
			return;
		}

		BPath tempDir;
		if (find_directory(B_SYSTEM_TEMP_DIRECTORY, &tempDir) != B_OK) {
			PRINT(("TempCDCopy: can't find temp directory, "
				"encoding directly from CD\n"));
			return;
		}
		tempDir.Append("Hare-rip");
		if (create_directory(tempDir.Path(), 0777) != B_OK) {
			PRINT(("TempCDCopy: can't create temp directory, "
				"encoding directly from CD\n"));
			return;
		}

		BString tempName;
		tempName << find_thread(NULL) << "-" << system_time()
			<< "-" << ref->name;
		BPath tempPath(tempDir.Path(), tempName.String());

		PRINT(("TempCDCopy: copying %s to %s\n", originalPath.Path(),
			tempPath.Path()));
		status_t copyStatus = Copy(originalPath.Path(), tempPath.Path(),
			progressMessenger);
		if (copyStatus == B_OK) {
			fPath = tempPath;
			fIsTemp = true;
		} else {
			if (copyStatus == FSS_CANCEL_ENCODING) {
				PRINT(("TempCDCopy: copy canceled\n"));
				fCanceled = true;
			} else {
				PRINT(("TempCDCopy: copy failed, encoding directly from CD\n"));
			}
			// Tidy up the partial copy either way - a canceled rip and a
			// failed one both leave an incomplete file behind otherwise.
			BEntry(tempPath.Path()).Remove();
		}
	}

	~TempCDCopy()
	{
		if (fIsTemp) {
			BEntry(fPath.Path()).Remove();
		}
	}

	const char* Path() const { return fPath.Path(); }
	bool WasCanceled() const { return fCanceled; }

private:
	// Same poll AEEncoder::CheckForCancel() does against this same
	// "_Encoder_" thread's data queue - Copy() runs on that thread too, so
	// find_thread(NULL) here is that thread.
	static bool IsCanceled()
	{
		thread_id thread = find_thread(NULL);
		if (has_data(thread)) {
			thread_id sender;
			int32 code = receive_data(&sender, 0, 0);
			if (code == FSS_CANCEL_ENCODING) {
				return true;
			}
		}
		return false;
	}

	static status_t Copy(const char* srcPath, const char* dstPath,
		BMessenger* progressMessenger)
	{
		BFile src(srcPath, B_READ_ONLY);
		status_t status = src.InitCheck();
		if (status != B_OK) {
			return status;
		}

		BFile dst(dstPath, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		status = dst.InitCheck();
		if (status != B_OK) {
			return status;
		}

		off_t totalSize = 0;
		bool haveTotalSize = (src.GetSize(&totalSize) == B_OK)
			&& (totalSize > 0);
		bool reportProgress = progressMessenger
			&& progressMessenger->IsValid() && haveTotalSize;

		// Heap-allocated rather than a stack buffer: this runs on the
		// spawned "_Encoder_" thread, which doesn't get a large stack, and
		// 256 KB of locals here was enough to overflow it (crashing inside
		// this function, or in whichever caller's frame pushed it over).
		const size_t bufferSize = 262144; // 256 KB - large, sequential I/O
		char* buffer = new char[bufferSize];

		status_t result = B_OK;
		ssize_t bytesRead;
		off_t bytesCopied = 0;
		float prevPercent = 0.0f;
		while ((bytesRead = src.Read(buffer, bufferSize)) > 0) {
			// Checked once per chunk (same as the encoders' own status
			// polling) rather than just leaving cancellation to be noticed
			// once the whole track has been copied and handed off to the
			// encoder - a slow rip otherwise looks unresponsive to Abort.
			if (IsCanceled()) {
				PRINT(("TempCDCopy: cancel requested mid-copy\n"));
				result = FSS_CANCEL_ENCODING;
				break;
			}

			ssize_t bytesWritten = dst.Write(buffer, bytesRead);
			if (bytesWritten != bytesRead) {
				result = B_IO_ERROR;
				break;
			}

			if (reportProgress) {
				bytesCopied += bytesRead;
				float percent = (100.0f * bytesCopied) / totalSize;
				BMessage updateMsg(B_UPDATE_STATUS_BAR);
				updateMsg.AddFloat("delta", percent - prevPercent);
				progressMessenger->SendMessage(&updateMsg);
				prevPercent = percent;
			}
		}
		if ((result == B_OK) && (bytesRead < 0)) {
			result = (status_t)bytesRead;
		}

		delete[] buffer;
		return result;
	}

	BPath fPath;
	bool fIsTemp;
	bool fCanceled;
};

// Small RAII holder for the cover art bytes EncodeThread() captures (once,
// under the looper lock) from the view right at the start of a run. Freed
// automatically by the destructor on every one of EncodeThread()'s many
// early-return paths, the same reasoning TempCDCopy above uses for the rip
// temp file - saves duplicating cleanup at each return site.
class CoverArtBuffer {
public:
	CoverArtBuffer()
		:
		fData(NULL),
		fSize(0)
	{
	}

	~CoverArtBuffer()
	{
		delete[] fData;
	}

	void SetTo(const void* data, size_t size, const BString& extension)
	{
		delete[] fData;
		fData = NULL;
		fSize = 0;
		if (data && (size > 0)) {
			fData = new unsigned char[size];
			memcpy(fData, data, size);
			fSize = size;
		}
		fExtension = extension;
	}

	bool HasData() const { return (fData != NULL) && (fSize > 0); }
	const void* Data() const { return fData; }
	size_t Size() const { return fSize; }
	const BString& Extension() const { return fExtension; }

private:
	unsigned char* fData;
	size_t fSize;
	BString fExtension;
};

} // namespace

namespace {

// Rough free-space guesses for the low-disk-space warning below - not
// meant to be exact, just enough to catch "you're about to run out"
// before a long encode run fails partway through and leaves partial
// output files behind. FLAC's guess scales with how many tracks are
// about to be encoded, since its files run far bigger than a compressed
// MP3/Ogg track; MP3/Ogg get one flat threshold instead, since their
// files stay small enough that dropping below it means something's
// already wrong regardless of how many tracks are queued up.
const off_t kLowSpaceThresholdOggMp3 = 150LL * 1024 * 1024;
const off_t kFlacBytesPerTrackGuess = 50LL * 1024 * 1024;

// Walks up from path until it reaches a directory that actually exists -
// the output folder (or several levels of it) may not have been created
// yet, since EncodeThread() only creates the whole chain with
// create_directory() right before encoding - then returns the volume
// that existing ancestor lives on. Encoded output always lands somewhere
// under that same tree, so its free space is the right thing to check.
BVolume
VolumeForOutputPath(BPath path)
{
	BEntry entry(path.Path());
	while (!entry.Exists()) {
		BPath parent;
		if ((path.GetParent(&parent) != B_OK)
				|| (strcmp(parent.Path(), path.Path()) == 0)) {
			break;
		}
		path = parent;
		entry.SetTo(path.Path());
	}

	entry_ref ref;
	if (entry.GetRef(&ref) != B_OK) {
		return BVolume();
	}

	return BVolume(ref.device);
}

} // namespace

// Best-effort low-disk-space warning, checked only when the user presses
// Encode (never during the encode run itself). Only actually pops up the
// warning the first time it's checked, or if free space has dropped
// further since the last time it was checked - so pressing Encode
// repeatedly while still low, but not any lower, doesn't nag every time.
void
AppView::CheckDiskSpace()
{
	PRINT(("AppView::CheckDiskSpace()\n"));

	int32 numRows = listView->CountRows();
	if (numRows == 0) {
		return;
	}

	int32 numSelected = 0;
	for (int i = 0; i < numRows; i++) {
		if (listView->RowAt(i)->IsSelected()) {
			numSelected++;
		}
	}
	int32 tracksToEncode = ((numSelected == 0) || (numSelected == numRows))
		? numRows : numSelected;

	// Any row about to be encoded works for figuring out the destination
	// volume - they virtually always share one, and this is only meant
	// as a heads-up, not a guarantee.
	BRefRow* row = NULL;
	for (int i = 0; i < numRows; i++) {
		BRefRow* candidate = (BRefRow*)listView->RowAt(i);
		if ((numSelected == 0) || (numSelected == numRows)
				|| candidate->IsSelected()) {
			row = candidate;
			break;
		}
	}
	if (!row) {
		return;
	}

	BStringField* outputField = (BStringField*)row->GetField(SAVE_AS_COLUMN_INDEX);
	if (!outputField || !outputField->String()) {
		return;
	}

	BPath outputPath(outputField->String());
	BPath outputParent;
	if ((outputPath.InitCheck() != B_OK)
			|| (outputPath.GetParent(&outputParent) != B_OK)) {
		return;
	}

	BVolume volume = VolumeForOutputPath(outputParent);
	if (volume.InitCheck() != B_OK) {
		return;
	}
	off_t freeBytes = volume.FreeBytes();

	AEEncoder* encoder = settings->Encoder();
	BString encoderName(encoder ? encoder->GetName() : "");
	encoderName.ToLower();
	bool isFlac = (encoderName.FindFirst("flac") >= 0);

	off_t neededBytes = isFlac
		? ((off_t)tracksToEncode * kFlacBytesPerTrackGuess)
		: kLowSpaceThresholdOggMp3;

	bool lowOnSpace = (freeBytes < neededBytes);
	bool firstCheck = (fLastCheckedFreeBytes < 0);
	bool worseThanLastTime = (freeBytes < fLastCheckedFreeBytes);

	if (lowOnSpace && (firstCheck || worseThanLastTime)) {
		BString msg("Low disk space: only about ");
		msg << (freeBytes / (1024 * 1024));
		msg << " MB free on the destination volume, but this encode run "
			"may need roughly ";
		msg << (neededBytes / (1024 * 1024));
		msg << " MB.\n\nYou can close this and continue if you like - "
			"this is just a warning.";
		AlertUser(msg.String());
	}

	fLastCheckedFreeBytes = freeBytes;
}

void
AppView::Encode()
{
	PRINT(("AppView::Encode()\n"));

	CheckDiskSpace();

	thread_id thread = spawn_thread(AppView::EncodeThread, "_Encoder_",
									B_NORMAL_PRIORITY, (void*)this);
	resume_thread(thread);
}

int32
AppView::EncodeThread(void* args)
{
	PRINT(("AppView::EncodeThread(void*)\n"));

	settings->SetEncoding(true);
	AppView* view = (AppView*)args;
	BMenuBar* menuBar = ((AppWindow*)view->Window())->MenuBar();

	AEEncoder* encoder = settings->Encoder();
	if (!encoder || (encoder->InitCheck() != B_OK)) {
		if (encoder->InitCheck() == FSS_EXE_NOT_FOUND) {
			PRINT(("ERROR: exe not found\n"));
			BString msg("The addon could not find the required executable.");
			msg << "\n";
			msg << "Please check the addon's documentation.\n";
			view->AlertUser(msg.String());
		}
		system_beep(SYSTEM_BEEP_ENCODING_DONE);
		return B_ERROR;
	}

	encoder->InitEncoder();
	BMessenger statusBarMessenger(view->statusBar);

	if (view->LockLooper()) {
		view->editorView->SetEnabled(false);
		view->encodeButton->SetEnabled(false);
		view->cancelButton->SetLabel(ABORT_BTN);
		view->cancelButton->SetEnabled(true);
		view->Invalidate();
		menuBar->SetEnabled(false);
		menuBar->Invalidate();
		view->UnlockLooper();
	}

	// Captured once, up front, under the looper lock - the cover art (if
	// any) found for whichever disc is currently loaded doesn't change
	// mid-encode, so there's no need to keep re-locking to read it as each
	// track finishes.
	CoverArtBuffer coverArt;
	BString mbDiscId, mbReleaseId, mbReleaseGroupId, mbArtistId;
	BObjectList<TrackMBMetadata, true> mbTrackMetadata(20);
	if (view->LockLooper()) {
		coverArt.SetTo(view->coverArtImageData, view->coverArtImageSize,
			view->coverArtImageExt);
		mbDiscId = view->mbDiscId;
		mbReleaseId = view->mbReleaseId;
		mbReleaseGroupId = view->mbReleaseGroupId;
		mbArtistId = view->mbArtistId;
		if (view->mbTrackMetadata) {
			for (int32 i = 0; i < view->mbTrackMetadata->CountItems(); i++) {
				TrackMBMetadata* src = view->mbTrackMetadata->ItemAt(i);
				if (!src) {
					continue;
				}
				TrackMBMetadata* copy = new TrackMBMetadata(*src);
				mbTrackMetadata.AddItem(copy);
			}
		}
		view->UnlockLooper();
	}

	BObjectList<BRefRow> objList;
	BRefRow* row;
	BBitmapField* tmpBitmapField;
	BStringField* tmpStringField;

	int32 numRows = view->listView->CountRows();
	int32 numSelected = 0;
	for (int i = 0; i < numRows; i++) {
		if (view->listView->RowAt(i)->IsSelected()) {
			numSelected++;
		}
	}
	if (view->LockLooper()) {
		if ((numSelected == 0) || (numSelected == numRows)) {
		// if none are selected, or all are selected, then encode all
			for (int i = 0; i < numRows; i++) {
				row = (BRefRow*)view->listView->RowAt(i);
				row->SetField(new BBitmapField((BBitmap*)NULL), COMPLETE_COLUMN_INDEX);
				objList.AddItem(row);
				view->listView->InvalidateRow(row);
			}
		} else {
		// otherwise only encode the selected ones
			for (int i = 0; i < numRows; i++) {
				if (view->listView->RowAt(i)->IsSelected()) {
					row = (BRefRow*)view->listView->RowAt(i);
					row->SetField(new BBitmapField((BBitmap*)NULL), COMPLETE_COLUMN_INDEX);
					objList.AddItem(row);
					view->listView->InvalidateRow(row);
				}
			}
			numRows = numSelected;
		}
		view->UnlockLooper();
	}


	for (int i = 0; i < numRows; i++) {
		row = objList.ItemAt(i);
		
		tmpStringField = (BStringField*)row->GetField(ARTIST_COLUMN_INDEX);
		const char* artist = tmpStringField->String();
		tmpStringField = (BStringField*)row->GetField(ALBUM_COLUMN_INDEX);
		const char* album = tmpStringField->String();
		tmpStringField = (BStringField*)row->GetField(TITLE_COLUMN_INDEX);
		const char* title = tmpStringField->String();
		tmpStringField = (BStringField*)row->GetField(YEAR_COLUMN_INDEX);
		const char* year = tmpStringField->String();
		tmpStringField = (BStringField*)row->GetField(COMMENT_COLUMN_INDEX);
		const char* comment = tmpStringField->String();
		tmpStringField = (BStringField*)row->GetField(TRACK_COLUMN_INDEX);
		const char* track = tmpStringField->String();
		tmpStringField = (BStringField*)row->GetField(GENRE_COLUMN_INDEX);
		const char* genre = tmpStringField->String();
		int32 genreNum;
		if (genre) {
			genreNum = GenreList::Genre(genre);
		}
		tmpStringField = (BStringField*)row->GetField(SAVE_AS_COLUMN_INDEX);
		const char* outputFile = tmpStringField->String();
		entry_ref* ref = row->EntryRef();
		BEntry entry(ref);
		if (entry.InitCheck() != B_OK) {
			PRINT(("ERROR: entry failed InitCheck()\n"));
			BString msg("Error opening file: ");
			msg << ref->name;
			view->AlertUser(msg.String());
			if (view->LockLooper()) {
				BString remaining(STATUS_TRAILING_LABEL);
				remaining << 0;
				view->statusBar->Reset(STATUS_LABEL, remaining.String());
				view->editorView->SetEnabled(true);
				view->encodeButton->SetEnabled(true);
				view->cancelButton->SetLabel(CANCEL_BTN);
				view->cancelButton->SetEnabled(true);
				view->Invalidate();
				menuBar->SetEnabled(true);
				menuBar->Invalidate();
				view->UnlockLooper();
			}
			settings->SetEncoding(false);
			encoder->UninitEncoder();
			system_beep(SYSTEM_BEEP_ENCODING_DONE);
			return B_ERROR;
		}

		BPath path;
		entry.GetPath(&path);
		if (path.InitCheck() != B_OK) {
			PRINT(("ERROR: path failed InitCheck()\n"));
			BString msg("Error opening file: ");
			msg << ref->name;
			view->AlertUser(msg.String());
			if (view->LockLooper()) {
				BString remaining(STATUS_TRAILING_LABEL);
				remaining << 0;
				view->statusBar->Reset(STATUS_LABEL, remaining.String());
				view->editorView->SetEnabled(true);
				view->encodeButton->SetEnabled(true);
				view->cancelButton->SetLabel(CANCEL_BTN);
				view->cancelButton->SetEnabled(true);
				view->Invalidate();
				menuBar->SetEnabled(true);
				menuBar->Invalidate();
				view->UnlockLooper();
			}
			settings->SetEncoding(false);
			encoder->UninitEncoder();
			system_beep(SYSTEM_BEEP_ENCODING_DONE);
			return B_ERROR;
		}

		BPath outputPath(outputFile);
		if (outputPath.InitCheck() != B_OK) {
			PRINT(("ERROR: outputPath failed InitCheck()\n"));
			BString msg("Error creating path for: ");
			msg << outputFile;
			view->AlertUser(msg.String());
			if (view->LockLooper()) {
				BString remaining(STATUS_TRAILING_LABEL);
				remaining << 0;
				view->statusBar->Reset(STATUS_LABEL, remaining.String());
				view->editorView->SetEnabled(true);
				view->encodeButton->SetEnabled(true);
				view->cancelButton->SetLabel(CANCEL_BTN);
				view->cancelButton->SetEnabled(true);
				view->Invalidate();
				menuBar->SetEnabled(true);
				menuBar->Invalidate();
				view->UnlockLooper();
			}
			settings->SetEncoding(false);
			encoder->UninitEncoder();
			system_beep(SYSTEM_BEEP_ENCODING_DONE);
			return B_ERROR;
		}
		outputFile = outputPath.Path();
		BPath outputParent;
		outputPath.GetParent(&outputParent);
		if (outputParent.InitCheck() != B_OK) {
			PRINT(("ERROR: outputParent failed InitCheck()\n"));
			BString msg("Error creating path for: ");
			msg << outputFile;
			view->AlertUser(msg.String());
			if (view->LockLooper()) {
				BString remaining(STATUS_TRAILING_LABEL);
				remaining << 0;
				view->statusBar->Reset(STATUS_LABEL, remaining.String());
				view->editorView->SetEnabled(true);
				view->encodeButton->SetEnabled(true);
				view->cancelButton->SetLabel(CANCEL_BTN);
				view->cancelButton->SetEnabled(true);
				view->Invalidate();
				menuBar->SetEnabled(true);
				menuBar->Invalidate();
				view->UnlockLooper();
			}
			settings->SetEncoding(false);
			encoder->UninitEncoder();
			system_beep(SYSTEM_BEEP_ENCODING_DONE);
			return B_ERROR;
		}
		if (create_directory(outputParent.Path(), 0777) != B_OK) {
			PRINT(("ERROR: failed to create directory\n"));
			BString msg("Error creating path for: ");
			msg << outputFile;
			view->AlertUser(msg.String());
			if (view->LockLooper()) {
				BString remaining(STATUS_TRAILING_LABEL);
				remaining << 0;
				view->statusBar->Reset(STATUS_LABEL, remaining.String());
				view->editorView->SetEnabled(true);
				view->encodeButton->SetEnabled(true);
				view->cancelButton->SetLabel(CANCEL_BTN);
				view->cancelButton->SetEnabled(true);
				view->Invalidate();
				menuBar->SetEnabled(true);
				menuBar->Invalidate();
				view->UnlockLooper();
			}
			settings->SetEncoding(false);
			encoder->UninitEncoder();
			system_beep(SYSTEM_BEEP_ENCODING_DONE);
			return B_ERROR;
		}

		// Ripping the track to a local temp file first (when it's coming
		// from a live audio CD) and encoding from that copy instead of
		// reading straight off the disc. tempCopy's destructor cleans the
		// temp file up at the end of this iteration regardless of which
		// path out of the loop body below gets taken. This copy happens
		// synchronously below, so post a status update first - otherwise
		// the bar would just sit on the previous track's label for
		// however long the rip takes, looking like a stall.
		if (view->LockLooper()) {
			BString ripping(RIPPING_LABEL);
			ripping << path.Leaf();
			BString remaining(STATUS_TRAILING_LABEL);
			remaining << (numRows - i);
			view->statusBar->Reset(ripping.String(), remaining.String());
			view->UnlockLooper();
		}
		TempCDCopy tempCopy(ref, path, &statusBarMessenger);
		if (tempCopy.WasCanceled()) {
			// Same cleanup as the FSS_CANCEL_ENCODING case below - the
			// difference is this cancellation happened during the rip,
			// before there was an encoder output file to speak of yet, but
			// outputFile/outputParent are already known at this point in
			// the loop either way, and TempCDCopy has already removed its
			// own (partial) temp copy in its destructor/constructor.
			PRINT(("User canceled during CD rip.\n"));
			if (view->LockLooper()) {
				BString remaining(STATUS_TRAILING_LABEL);
				remaining << 0;
				view->statusBar->Reset(STATUS_LABEL, remaining.String());
				view->editorView->SetEnabled(true);
				view->encodeButton->SetEnabled(true);
				view->cancelButton->SetLabel(CANCEL_BTN);
				view->cancelButton->SetEnabled(true);
				view->Invalidate();
				menuBar->SetEnabled(true);
				menuBar->Invalidate();
				view->UnlockLooper();
			}
			settings->SetEncoding(false);
			encoder->UninitEncoder();
			BEntry outputEntry(outputFile);
			outputEntry.Remove();
			while (1) {
				BDirectory directory(outputParent.Path());
				if (directory.CountEntries() > 0) {
					break;
				}
				directory.Unset();
				BEntry dirEntry(outputParent.Path());
				dirEntry.Remove();
				dirEntry.Unset();
				outputParent.GetParent(&outputParent);
			}
			system_beep(SYSTEM_BEEP_ENCODING_DONE);
			return B_OK;
		}
		const char* inputFile = tempCopy.Path();

		if (view->LockLooper()) {
			BString encoding(STATUS_LABEL);
			encoding << path.Leaf();
			BString remaining(STATUS_TRAILING_LABEL);
			remaining << (numRows - i);
			view->statusBar->Reset(encoding.String(), remaining.String());
			view->UnlockLooper();
		}

		BMessage encodeMessage(FSS_ENCODE);
		encodeMessage.AddString("input file", inputFile);
		encodeMessage.AddString("output file", outputFile);
		encodeMessage.AddMessenger("statusBarMessenger", statusBarMessenger);
		if (artist) {
			encodeMessage.AddString("artist", artist);
		}
		if (album) {
			encodeMessage.AddString("album", album);
		}
		if (title) {
			encodeMessage.AddString("title", title);
		}
		if (year) {
			encodeMessage.AddString("year", year);
		}
		if (comment) {
			encodeMessage.AddString("comment", comment);
		}
		if (track) {
			encodeMessage.AddString("track", track);
		}
		if (genre) {
			encodeMessage.AddString("genre", genre);
			encodeMessage.AddInt32("genre number", genreNum);
		}

		PRINT(("Calling Addon: %s\n", encoder->GetName()));
		int32 retValue = encoder->Encode(&encodeMessage);
		switch (retValue) {
			case B_OK:
				PRINT(("Addon returned successfully.\n"));
				break;
			case FSS_EXE_NOT_FOUND: {
					PRINT(("ERROR: exe not found\n"));
					BString msg("The addon could not find the required executable.");
					msg << "\n";
					msg << "Please check the addon's dccumentation.\n";
					view->AlertUser(msg.String());
					if (view->LockLooper()) {
						BString remaining(STATUS_TRAILING_LABEL);
						remaining << 0;
						view->statusBar->Reset(STATUS_LABEL, remaining.String());
						view->editorView->SetEnabled(true);
						view->encodeButton->SetEnabled(true);
						view->cancelButton->SetLabel(CANCEL_BTN);
						view->cancelButton->SetEnabled(true);
						view->Invalidate();
						menuBar->SetEnabled(true);
						menuBar->Invalidate();
						view->UnlockLooper();
					}
					settings->SetEncoding(false);
					encoder->UninitEncoder();
					system_beep(SYSTEM_BEEP_ENCODING_DONE);
				}
				return B_ERROR;
			case FSS_INPUT_NOT_SUPPORTED: {
					PRINT(("ERROR: input not supported\n"));
					BString msg("Input File: ");
					msg << ref->name;
					msg << " cannot be encoded with this encoder. Continue?";
					BAlert* alert = new BAlert("alert", msg.String(), YES, NO);
					int32 button = alert->Go();
					if (button == 1) {
						if (view->LockLooper()) {
							BString remaining(STATUS_TRAILING_LABEL);
							remaining << 0;
							view->statusBar->Reset(STATUS_LABEL, remaining.String());
							view->editorView->SetEnabled(true);
							view->encodeButton->SetEnabled(true);
							view->cancelButton->SetLabel(CANCEL_BTN);
							view->cancelButton->SetEnabled(true);
							view->Invalidate();
							menuBar->SetEnabled(true);
							menuBar->Invalidate();
							view->UnlockLooper();
						}
						settings->SetEncoding(false);
						encoder->UninitEncoder();
						system_beep(SYSTEM_BEEP_ENCODING_DONE);
						return B_ERROR;
					} else {
						continue;
					}
				}
			case FSS_CANCEL_ENCODING: {
					PRINT(("User canceled encoding.\n"));
					if (view->LockLooper()) {
						BString remaining(STATUS_TRAILING_LABEL);
						remaining << 0;
						view->statusBar->Reset(STATUS_LABEL, remaining.String());
						view->editorView->SetEnabled(true);
						view->encodeButton->SetEnabled(true);
						view->cancelButton->SetLabel(CANCEL_BTN);
						view->cancelButton->SetEnabled(true);
						view->Invalidate();
						menuBar->SetEnabled(true);
						menuBar->Invalidate();
						view->UnlockLooper();
					}
					settings->SetEncoding(false);
					encoder->UninitEncoder();
					BEntry outputEntry(outputFile);
					outputEntry.Remove();
					while (1) {
						BDirectory directory(outputParent.Path());
						if (directory.CountEntries() > 0) {
							break;
						}
						directory.Unset();
						BEntry dirEntry(outputParent.Path());
						dirEntry.Remove();
						dirEntry.Unset();
						outputParent.GetParent(&outputParent);
					}
					system_beep(SYSTEM_BEEP_ENCODING_DONE);
				}
				return B_OK;
			case B_ERROR: {
					PRINT(("ERROR: encoding failed\n"));
					const char* errmsg;
					encodeMessage.FindString("error", &errmsg);
					BString msg("Error encoding file: ");
					msg << ref->name;
					msg << "\n";
					msg << errmsg;
					msg << "Cannot continue.";
					view->AlertUser(msg.String());
					if (view->LockLooper()) {
						BString remaining(STATUS_TRAILING_LABEL);
						remaining << 0;
						view->statusBar->Reset(STATUS_LABEL, remaining.String());
						view->editorView->SetEnabled(true);
						view->encodeButton->SetEnabled(true);
						view->cancelButton->SetLabel(CANCEL_BTN);
						view->cancelButton->SetEnabled(true);
						view->Invalidate();
						menuBar->SetEnabled(true);
						menuBar->Invalidate();
						view->UnlockLooper();
					}
					settings->SetEncoding(false);
					encoder->UninitEncoder();
					system_beep(SYSTEM_BEEP_ENCODING_DONE);
				}
				return B_ERROR;
		}

		if (view->LockLooper()) {
			BBitmap* checkMark = view->listView->GetCheckMark();
			tmpBitmapField = (BBitmapField*)row->GetField(COMPLETE_COLUMN_INDEX);
			tmpBitmapField->SetBitmap(checkMark);
			view->listView->InvalidateRow(row);
			view->UnlockLooper();
		}

		// Cover art attaching is deliberately just "drop a cover file
		// next to the encoded output" for now - embedding it into the
		// encoded files' own tags/attributes is a later task.
		if (coverArt.HasData()) {
			view->WriteCoverArt(outputParent, coverArt.Data(), coverArt.Size(),
				coverArt.Extension());
		}

		// The standard Audio:*/tag fields (artist/album/title/etc.) are
		// already written by the encoder addon itself, as part of
		// encoder->Encode() above - this only adds the MusicBrainz-
		// specific IDs, which is a no-op when mbDiscId is empty (no
		// MusicBrainz metadata for this disc).
		view->WriteMusicBrainzMetadata(outputPath, track ? atol(track) : 0,
			mbDiscId, mbReleaseId, mbReleaseGroupId, mbArtistId,
			&mbTrackMetadata);
	}

	encoder->UninitEncoder();
	settings->SetEncoding(false);

	if (view->LockLooper()) {
		BString remaining(STATUS_TRAILING_LABEL);
		remaining << 0;
		view->statusBar->Reset(STATUS_LABEL, remaining.String());
		view->editorView->SetEnabled(true);
		view->encodeButton->SetEnabled(true);
		view->cancelButton->SetLabel(CANCEL_BTN);
		view->cancelButton->SetEnabled(true);
		view->Invalidate();
		menuBar->SetEnabled(true);
		menuBar->Invalidate();
		view->UnlockLooper();
	}

	system_beep(SYSTEM_BEEP_ENCODING_DONE);
	return B_OK;
}

void
AppView::SaveLayout()
{
	PRINT(("AppView::SaveLayout()\n"));

	BMessage layout;

	if (Window()) {
		layout.AddRect("windowFrame", Window()->Frame());
	}

	// Splitter positions aren't tracked anywhere as a single "position"
	// value - what actually reflects where the user left them is each
	// side's current on-screen size, so that's what gets saved (and later
	// handed straight back to BSplitView::SetItemWeight() as the weight,
	// which works fine since only the ratio between the two matters).
	if (topSplitView && (topSplitView->CountChildren() == 2)) {
		layout.AddFloat("topSplitWeight", topSplitView->ChildAt(0)->Frame().Width());
		layout.AddFloat("topSplitWeight", topSplitView->ChildAt(1)->Frame().Width());
	}

	if (mainSplitView && (mainSplitView->CountChildren() == 2)) {
		layout.AddFloat("mainSplitWeight", mainSplitView->ChildAt(0)->Frame().Height());
		layout.AddFloat("mainSplitWeight", mainSplitView->ChildAt(1)->Frame().Height());
	}

	settings->SetLayoutState(&layout);
	settings->SaveSettings();
}

void
AppView::RestoreLayout()
{
	PRINT(("AppView::RestoreLayout()\n"));

	BMessage* layout = settings->LayoutState();

	float weight0, weight1;
	if (topSplitView
			&& (layout->FindFloat("topSplitWeight", 0, &weight0) == B_OK)
			&& (layout->FindFloat("topSplitWeight", 1, &weight1) == B_OK)) {
		topSplitView->SetItemWeight(0, weight0, false);
		topSplitView->SetItemWeight(1, weight1, true);
	}

	if (mainSplitView
			&& (layout->FindFloat("mainSplitWeight", 0, &weight0) == B_OK)
			&& (layout->FindFloat("mainSplitWeight", 1, &weight1) == B_OK)) {
		mainSplitView->SetItemWeight(0, weight0, false);
		mainSplitView->SetItemWeight(1, weight1, true);
	}
}

void
AppView::Cancel()
{
	PRINT(("AppView::Cancel()\n"));

	if (settings->IsEncoding()) {
		thread_id encoder = find_thread("_Encoder_");
		if (!has_data(encoder)) {
			send_data(encoder, FSS_CANCEL_ENCODING, 0, 0);
		}
		cancelButton->SetEnabled(false);
	}
}

void
AppView::AlertUser(const char* message)
{
	PRINT(("AppView::AlertUser(const char*)\n"));
	BAlert* alert = new BAlert("alert", message, OK, NULL, NULL, B_WIDTH_AS_USUAL,
							   B_WARNING_ALERT);
	alert->Go();
}

void
AppView::WriteCoverArt(const BPath& directory, const void* data, size_t size,
	const BString& extension)
{
	PRINT(("AppView::WriteCoverArt(const BPath&, const void*, size_t, "
		"const BString&)\n"));

	if (!data || (size == 0)) {
		return;
	}

	BString fileName("cover.");
	fileName << extension;
	BPath coverPath(directory.Path(), fileName.String());
	if (coverPath.InitCheck() != B_OK) {
		return;
	}

	// Every track of an album typically shares the same output directory,
	// so this runs once per finished track but should only actually write
	// the file the first time - and leaves it alone if the user (or an
	// earlier encode) already has one there.
	BEntry existing(coverPath.Path());
	if (existing.Exists()) {
		return;
	}

	BFile file(coverPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK) {
		PRINT(("AppView::WriteCoverArt: couldn't create %s\n",
			coverPath.Path()));
		return;
	}

	file.Write(data, size);
}

void
AppView::WriteMusicBrainzMetadata(const BPath& outputPath, int32 trackNumber,
	const BString& discId, const BString& releaseId,
	const BString& releaseGroupId, const BString& artistId,
	const BObjectList<TrackMBMetadata, true>* trackMetadata)
{
	PRINT(("AppView::WriteMusicBrainzMetadata(const BPath&, int32, "
		"const BString&, ...)\n"));

	if (discId.Length() == 0) {
		// No MusicBrainz metadata for the currently loaded disc - either
		// the lookup found nothing, or it's gated off (see
		// ENABLE_MUSICBRAINZ_LOOKUP in AppDefs.h).
		return;
	}

	BString recordingId;
	if (trackMetadata) {
		for (int32 i = 0; i < trackMetadata->CountItems(); i++) {
			TrackMBMetadata* track = trackMetadata->ItemAt(i);
			if (track && (track->number == trackNumber)) {
				recordingId = track->recordingId;
				break;
			}
		}
	}

	// Haiku attributes, via the same AudioAttribute machinery
	// AudioAttributes already uses for the standard Audio:* ones (see
	// MusicBrainzAttributes.h) - self-registers each attribute's MIME
	// attr-info so Tracker's Attributes menu shows them.
	BFile file(outputPath.Path(), B_READ_WRITE);
	if (file.InitCheck() == B_OK) {
		MusicBrainzAttributes attributes(&file);
		attributes.SetDiscId(discId.String());
		attributes.SetReleaseId(releaseId.String());
		attributes.SetReleaseGroupId(releaseGroupId.String());
		attributes.SetArtistId(artistId.String());
		attributes.SetRecordingId(recordingId.String());
		if (attributes.Write() != B_OK) {
			PRINT(("AppView::WriteMusicBrainzMetadata: writing "
				"attributes failed for %s\n", outputPath.Path()));
		}
	} else {
		PRINT(("AppView::WriteMusicBrainzMetadata: couldn't open %s for "
			"attribute writing\n", outputPath.Path()));
	}

	// The same five values, embedded as tags. TagLib::File::properties()/
	// setProperties() (the PropertyMap API) maps an arbitrary key like
	// "MUSICBRAINZ_DISCID" to an ID3v2 TXXX frame for MP3, or to a
	// same-named Vorbis comment field for OGG/FLAC - one code path for
	// all three formats. These are the same tag names MusicBrainz Picard
	// itself writes, so a file Hare tags here reads back the same way in
	// Picard (which recognizes MUSICBRAINZ_TRACKID on import and treats
	// the file as already matched) or any other MusicBrainz-aware tool.
	TagLib::FileRef fileRef(outputPath.Path());
	if (!fileRef.isNull() && fileRef.file()) {
		TagLib::PropertyMap properties = fileRef.file()->properties();
		properties.replace("MUSICBRAINZ_DISCID",
			TagLib::StringList(discId.String()));
		if (releaseId.Length() > 0) {
			properties.replace("MUSICBRAINZ_ALBUMID",
				TagLib::StringList(releaseId.String()));
		}
		if (releaseGroupId.Length() > 0) {
			properties.replace("MUSICBRAINZ_RELEASEGROUPID",
				TagLib::StringList(releaseGroupId.String()));
		}
		if (artistId.Length() > 0) {
			properties.replace("MUSICBRAINZ_ARTISTID",
				TagLib::StringList(artistId.String()));
		}
		if (recordingId.Length() > 0) {
			properties.replace("MUSICBRAINZ_TRACKID",
				TagLib::StringList(recordingId.String()));
		}
		fileRef.file()->setProperties(properties);
		if (!fileRef.save()) {
			PRINT(("AppView::WriteMusicBrainzMetadata: TagLib save "
				"failed for %s\n", outputPath.Path()));
		}
	} else {
		PRINT(("AppView::WriteMusicBrainzMetadata: TagLib couldn't open "
			"%s\n", outputPath.Path()));
	}
}


BString
AppView::ReplaceInvalidFileChars(BString filestr, int32 swaptype)
{
	// Swap types
	// 1) ASCII to Dash
	// 2) ASCII to Underscore
	// 3) ASCII to UTF8 close match
	switch (swaptype)
	{
		case 1:
			filestr.ReplaceAllChars(SLASH, DASH, 0);
			filestr.ReplaceAllChars(BACKSLASH, DASH, 0);
			filestr.ReplaceAllChars(ASTERISK, DASH, 0);
			filestr.ReplaceAllChars(QUOTE, DASH, 0);
			filestr.ReplaceAllChars(COLON, DASH, 0);
			filestr.ReplaceAllChars(QUESTIONMARK, DASH, 0);
			filestr.ReplaceAllChars(GREATERTHAN, DASH, 0);
			filestr.ReplaceAllChars(LESSTHAN, DASH, 0);
			filestr.ReplaceAllChars(PIPE, DASH, 0);
			break;
		case 2:
			filestr.ReplaceAllChars(SLASH,UNDERSCORE, 0);
			filestr.ReplaceAllChars(BACKSLASH,UNDERSCORE, 0);
			filestr.ReplaceAllChars(ASTERISK, UNDERSCORE, 0);
			filestr.ReplaceAllChars(QUOTE,UNDERSCORE, 0);
			filestr.ReplaceAllChars(COLON,UNDERSCORE, 0);
			filestr.ReplaceAllChars(QUESTIONMARK,UNDERSCORE, 0);
			filestr.ReplaceAllChars(GREATERTHAN,UNDERSCORE, 0);
			filestr.ReplaceAllChars(LESSTHAN,UNDERSCORE, 0);
			filestr.ReplaceAllChars(PIPE,UNDERSCORE, 0);
			break;
		case 3:
			filestr.ReplaceAllChars(SLASH,ALT_SLASH, 0);
			filestr.ReplaceAllChars(BACKSLASH,ALT_BACKSLASH, 0);
			filestr.ReplaceAllChars(ASTERISK, ALT_ASTERISK, 0);
			filestr.ReplaceAllChars(QUOTE,ALT_QUOTE, 0);
			filestr.ReplaceAllChars(COLON,ALT_COLON, 0);
			filestr.ReplaceAllChars(QUESTIONMARK,ALT_QUESTIONMARK, 0);
			filestr.ReplaceAllChars(GREATERTHAN,ALT_GREATERTHAN, 0);
			filestr.ReplaceAllChars(LESSTHAN,ALT_LESSTHAN, 0);
			filestr.ReplaceAllChars(PIPE,ALT_PIPE, 0);
			break;
	}
	return filestr;
}
