#include "AppView.h"

#include <string.h>

#include <Alert.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Button.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <ctype.h>
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

#include "AEEncoder.h"
#include "AppDefs.h"
#include "AppWindow.h"
#include "AudioAttributes.h"
#include "CommandConstants.h"
#include "CheckMark.h"
#include "CoverArtView.h"
#include "EditorView.h"
#include "EncoderListView.h"
#include "GenreList.h"
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
	BView("AppView", B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP)
{
	PRINT(("AppView::AppView(BRect)\n"));

}

AppView::~AppView()
{
	PRINT(("AppView::~AppView()\n"));

	stop_watching(this);
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

	BString defaultTitle(ref->name);
	int32 extensionIndex = defaultTitle.FindLast(".");
	if (extensionIndex >= 0) {
		defaultTitle.Truncate(extensionIndex);
	}
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
class TempCDCopy {
public:
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
		// spawned "_Encoder_" thread, 256 KB - large, sequential I/O from CD
		const size_t bufferSize = 262144;
		char* buffer = new char[bufferSize];

		status_t result = B_OK;
		ssize_t bytesRead;
		off_t bytesCopied = 0;
		float prevPercent = 0.0f;
		while ((bytesRead = src.Read(buffer, bufferSize)) > 0) {
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

} //namespace


void
AppView::Encode()
{
	PRINT(("AppView::Encode()\n"));

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
