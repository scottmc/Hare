/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __APP_VIEW_H__
#define __APP_VIEW_H__

#include <vector>

#include <Box.h>
#include <Node.h>
#include <ObjectList.h>
#include <String.h>
#include <View.h>

#include "tagkit/CoverArtImage.h"

#include "RefRow.h"

class BButton;
class BMessage;
class BPath;
class BRect;
class BSplitView;
class BStatusBar;
class BStringView;
class BTextControl;
class BarberPoleView;
class EditorView;
class EncoderListView;
class PrefWindow;

namespace tagkit {
class CoverArtPickerWindow;
class CoverArtView;
}

// One track's MusicBrainz metadata, as reported by MusicBrainzLookup's
// METADATA_FOUND message - kept around (one flat list per currently loaded
// disc) so EncodeThread() can look up the right track's Recording MBID (and
// MusicBrainz's own title/artist for that track) by track number as each
// one finishes encoding.
struct TrackMBMetadata {
	int32 number;
	BString title;
	BString artist;
	BString recordingId;
};

class AppView : public BView {
public:
	AppView();
	~AppView();
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);
	virtual void RefsReceived(BMessage* message);
	void SaveLayout();
	void RestoreLayout();
private:
	void InitView();
	void InitializeColumn(BRefRow* row);
	void SetSaveAsColumn(BRefRow* row);
	void ApplyAttributeChanges(BMessage* message);
	void RemoveNodeFromList(node_ref* ref);
	void RemoveDeviceItemsFromList(int32 device);
	BString ReplaceInvalidFileChars(BString filestr, int32 swaptype);
	int32 UpdateItem(BMessage* message);
	static int32 RefsRecievedWrapper(void* args);
	static int32 RemoveItemsFromList(void* args);
	void Encode();
	void CheckDiskSpace();
	bool ConfirmOverwrite();
	void Cancel();
	void AlertUser(const char* message);
	static int32 EncodeThread(void* args);
	void WriteCoverArt(const BPath& directory, const void* data, size_t size,
		const BString& extension);
	bool WriteTags(const BPath& outputPath, const BString& artist,
		const BString& album, const BString& title, const BString& year,
		const BString& comment, int32 trackNumber, const BString& genre,
		const void* coverData, size_t coverSize,
		const BString& coverExtension, const BString& discId,
		const BString& releaseId, const BString& releaseGroupId,
		const BString& artistId,
		const BObjectList<TrackMBMetadata, true>* trackMetadata,
		BString* errorMessage = NULL);
	void ClearCoverArtCandidates();
	// Shows coverArtImages[selectedCoverArt] in coverArtView.
	void ShowSelectedCoverArt();
	// Opens (or replaces) the picker window, when there's a choice to make.
	void ShowCoverArtPicker();
	void CloseCoverArtPicker();
	// The pop-up menu (Choose cover art...) for a right click on the art;
	// message is the COVER_ART_CONTEXT message with the click's "where".
	void ShowCoverArtMenu(BMessage* message);
	// Picks which of statusBoxView's two cards (statusBar, the normal
	// progress display, or barberPoleView, the indeterminate one) should
	// be visible right now, and starts/stops barberPoleView's animation
	// to match: the real progress bar always wins while an encode/rip is
	// actually running (settings->IsEncoding()), otherwise the barber
	// pole shows for as long as a MusicBrainz lookup is still in flight
	// (fMusicBrainzLookupsPending > 0). Called from EncodeThread() right
	// as it starts, and from MessageReceived()'s MUSICBRAINZ_LOOKUP_
	// STARTED/_FINISHED cases.
	void UpdateStatusCard();
	PrefWindow* prefWin;
	EncoderListView* listView;
	EditorView* editorView;
	BBox* editorBoxView;
	BScrollView* editorScrollView;
	tagkit::CoverArtView* coverArtView;
	BBox* coverArtBoxView;
	BSplitView* topSplitView;
	BSplitView* mainSplitView;
	BButton* encodeButton;
	BButton* cancelButton;
	BStatusBar* statusBar;
	// Holds statusBar and barberPoleView as two BCardLayout cards sharing
	// the same spot in the window - see UpdateStatusCard().
	BView* statusBoxView;
	BarberPoleView* barberPoleView;
	bool cancel;
	// Free space (in bytes) on the destination volume as of the last
	// time CheckDiskSpace() actually ran - see its own comment for how
	// this is used to avoid re-warning on every single Encode press.
	off_t fLastCheckedFreeBytes;
	// Count of MusicBrainzLookup runs currently in flight (usually 0 or
	// 1, but a second Load CD before the first lookup finishes stacks
	// another) - Encode stays disabled while this is above zero, so an
	// encode run can't start out from under a lookup that hasn't
	// delivered its cover art/metadata yet. See MUSICBRAINZ_LOOKUP_
	// STARTED/_FINISHED in MessageReceived().
	int32 fMusicBrainzLookupsPending;
	// Every cover-art candidate MusicBrainzLookup found for the currently
	// loaded disc's release (see MessageReceived()'s COVER_ART_FOUND case) -
	// almost always one entry, none until the first COVER_ART_FOUND
	// arrives. coverArtView shows coverArtImages[selectedCoverArt]; with
	// more than one candidate, coverArtPicker (TagKit's picker window, NULL
	// while closed) is where the user chooses between them, and
	// EncodeThread() writes out whichever one is selected when a run starts.
	std::vector<tagkit::CoverArtImage> coverArtImages;
	int32 selectedCoverArt;
	tagkit::CoverArtPickerWindow* coverArtPicker;
	BString mbDiscId;
	BString mbReleaseId;
	BString mbReleaseGroupId;
	BString mbArtistId;
	BObjectList<TrackMBMetadata, true>* mbTrackMetadata;
};

#endif
