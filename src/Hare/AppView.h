/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __APP_VIEW_H__
#define __APP_VIEW_H__

#include <Box.h>
#include <Node.h>
#include <ObjectList.h>
#include <String.h>
#include <View.h>

#include "RefRow.h"

class BButton;
class BMessage;
class BPath;
class BRect;
class BSplitView;
class BStatusBar;
class BStringView;
class BTextControl;
class CoverArtView;
class EditorView;
class EncoderListView;
class PrefWindow;
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
	void Cancel();
	void AlertUser(const char* message);
	static int32 EncodeThread(void* args);
	void WriteCoverArt(const BPath& directory, const void* data, size_t size,
		const BString& extension);
	void WriteMusicBrainzMetadata(const BPath& outputPath, int32 trackNumber,
		const BString& discId, const BString& releaseId,
		const BString& releaseGroupId, const BString& artistId,
		const BObjectList<TrackMBMetadata, true>* trackMetadata);
	PrefWindow* prefWin;
	EncoderListView* listView;
	EditorView* editorView;
	BBox* editorBoxView;
	BScrollView* editorScrollView;
	CoverArtView* coverArtView;
	BBox* coverArtBoxView;
	BSplitView* topSplitView;
	BSplitView* mainSplitView;
	BButton* encodeButton;
	BButton* cancelButton;
	BStatusBar* statusBar;
	bool cancel;
	// Free space (in bytes) on the destination volume as of the last
	// time CheckDiskSpace() actually ran - see its own comment for how
	// this is used to avoid re-warning on every single Encode press.
	off_t fLastCheckedFreeBytes;
	void* coverArtImageData;
	size_t coverArtImageSize;
	BString coverArtImageExt;
	BString mbDiscId;
	BString mbReleaseId;
	BString mbReleaseGroupId;
	BString mbArtistId;
	BObjectList<TrackMBMetadata, true>* mbTrackMetadata;
};

#endif
