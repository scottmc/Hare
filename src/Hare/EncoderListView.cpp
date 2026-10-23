/*
 * Copyright 2000-2021, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "EncoderListView.h"
#include "RefRow.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <Debug.h>
#include <ListView.h>
#include <Message.h>
#include <Rect.h>

#include <ColumnTypes.h>

#include "AppDefs.h"
#include "CheckMark.h"
#include "CommandConstants.h"
#include "GUIStrings.h"
#include "Settings.h"

namespace {

// The version of the column layout saved with the window's settings. Bump it
// when columns are added, removed or reordered: a layout saved by another
// version is then not loaded, since it would put the columns in their old
// places.
const int32 kColumnLayoutVersion = 3;


// A column of track numbers, which sorts by their value ("2" before "10"),
// with the rows that have none last.
class TrackNumberColumn : public BStringColumn {
public:
	TrackNumberColumn(const char* title, float width, float minWidth,
			float maxWidth, uint32 truncate, alignment align)
		:
		BStringColumn(title, width, minWidth, maxWidth, truncate, align)
	{
	}

	virtual int CompareFields(BField* field1, BField* field2)
	{
		int32 track1 = TrackOf(field1);
		int32 track2 = TrackOf(field2);
		if (track1 != track2)
			return track1 < track2 ? -1 : 1;
		return BStringColumn::CompareFields(field1, field2);
	}

private:
	static int32 TrackOf(BField* field)
	{
		const char* text = ((BStringField*)field)->String();
		int32 track = (text != NULL) ? atoi(text) : 0;
		return track > 0 ? track : INT_MAX;
	}
};

}	// namespace


EncoderListView::EncoderListView()
	:
	BColumnListView("listView",
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE | B_NAVIGABLE_JUMP,
		B_FANCY_BORDER, true)
{
	PRINT(("EncoderListView::EncoderListView(BRect)\n"));

	SetSelectionMode(B_MULTIPLE_SELECTION_LIST);

	InitView();
}

EncoderListView::~EncoderListView()
{
	PRINT(("EncoderListView::~EncoderListView()\n"));

	delete checkMark;

	SaveColumnState();
}

// The columns (their order, widths, which are shown, the sorting) go into
// the settings, to be saved with them; also done by "Save layout".
void
EncoderListView::SaveColumnState()
{
	BMessage columnState;
	SaveState(&columnState);
	columnState.AddInt32("columnLayoutVersion", kColumnLayoutVersion);
	settings->SetColumnsState(&columnState);
}

void
EncoderListView::InitView()
{
	PRINT(("EncoderListView::InitView()\n"));

	checkMark = new CheckMark();

	float minWidth;
	float maxWidth = 1000;
	int32 truncate = 3;
	float extraspace = 0;
	
	extraspace = StringWidth("WW"); // add extraspace equal to width of WW

	// The track number comes first, to the left of the check mark, and is
	// what the list is sorted by to begin with: the order of the disc, and
	// so the order the tracks are encoded in.
	// It starts wide enough for its title, but can be made as narrow as two
	// digits; the numbers are centered.
	float trackWidth = StringWidth(TRACK_COLUMN) + extraspace;
	float trackMinWidth = StringWidth("99") + 10;
	BColumn* trackColumn = new TrackNumberColumn(TRACK_COLUMN, trackWidth,
		trackMinWidth, maxWidth, truncate, B_ALIGN_CENTER);
	AddColumn(trackColumn, TRACK_COLUMN_INDEX);

	AddColumn(new BBitmapColumn(ICON_COLUMN, 16, 16, 16, B_ALIGN_CENTER), COMPLETE_COLUMN_INDEX);

	// The title comes before the file name: not every track is a song (a
	// comedy album, say), so it isn't called one.
	minWidth = StringWidth(TITLE_COLUMN) + extraspace;
	AddColumn(new BStringColumn(TITLE_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), TITLE_COLUMN_INDEX);

	minWidth = StringWidth(FILE_COLUMN) + extraspace;
	BColumn* fileColumn = new BStringColumn(FILE_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT);
	AddColumn(fileColumn, FILE_COLUMN_INDEX);

	minWidth = StringWidth(SAVE_AS_COLUMN) + extraspace;
	AddColumn(new BStringColumn(SAVE_AS_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), SAVE_AS_COLUMN_INDEX);

	minWidth = StringWidth(ARTIST_COLUMN) + extraspace;
	AddColumn(new BStringColumn(ARTIST_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), ARTIST_COLUMN_INDEX);

	minWidth = StringWidth(ALBUM_COLUMN) + extraspace;
	AddColumn(new BStringColumn(ALBUM_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), ALBUM_COLUMN_INDEX);

	minWidth = StringWidth(YEAR_COLUMN) + extraspace;
	AddColumn(new BStringColumn(YEAR_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), YEAR_COLUMN_INDEX);

	minWidth = StringWidth(COMMENT_COLUMN) + extraspace;
	AddColumn(new BStringColumn(COMMENT_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), COMMENT_COLUMN_INDEX);

	minWidth = StringWidth(GENRE_COLUMN) + extraspace;
	AddColumn(new BStringColumn(GENRE_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), GENRE_COLUMN_INDEX);

	// The layout saved last time (column order, widths, the sorting), unless
	// it was saved with another set of columns; then the sorting starts as
	// described above.
	int32 layoutVersion = 0;
	if (settings->ColumnsState()->FindInt32("columnLayoutVersion",
			&layoutVersion) == B_OK && layoutVersion == kColumnLayoutVersion) {
		LoadState(settings->ColumnsState());
	} else {
		SetSortColumn(trackColumn, false, true);
		// The title says more than the file name does, which is mostly the
		// same words; the File column is there to turn on.
		fileColumn->SetVisible(false);
	}
}


void
EncoderListView::AttachedToWindow()
{
	PRINT(("EncoderListView::AttachedToWindow()\n"));
}

void
EncoderListView::MessageDropped(BMessage* message, BPoint point)
{
	BMessenger messenger(Parent());
	messenger.SendMessage(message);
}

void
EncoderListView::MessageReceived(BMessage* message)
{
	PRINT(("EncoderListView::MessageReceived(BMessage*)\n"));

	switch (message->what) {
		default:
			BColumnListView::MessageReceived(message);
	}
}

void
EncoderListView::SelectionChanged()
{
	BMessage* listSelectMsg = new BMessage(LIST_SELECTION_MSG);
	listSelectMsg->AddPointer("source", this);
	
	for (int32 i = 0; i < CountRows(); i++) {
		if (RowAt(i)->IsSelected()) {
			listSelectMsg->AddInt32("index", i);
		}
	}
	
	BMessenger messenger(Parent());
	messenger.SendMessage(listSelectMsg);
	delete listSelectMsg;
}

void
EncoderListView::KeyDown(const char* bytes, int32 numBytes)
{
	uint8 byte = bytes[0];

	switch(byte)
	{
		case B_BACKSPACE:
			// fallthrough
		case B_DELETE:
			{
				for (int32 i = CountRows()-1; i >= 0; i--) {
					BRow* currentrow = this->RowAt(i);
					if (currentrow->IsSelected()) {
						RemoveRow(currentrow);
						delete currentrow;
					}
		        }
	        }
			break;
		default:
			break;
	}
}

BBitmap*
EncoderListView::GetCheckMark()
{
	return checkMark;
}
