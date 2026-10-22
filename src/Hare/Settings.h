/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __PREFERENCES_H__
#define __PREFERENCES_H__

#include <vector>

#include <Archivable.h>
#include <String.h>

#include "CommandConstants.h"

class AEEncoder;
class BMessage;

// The on/off preferences of the "Tagging and Attributes" box of the
// preferences window. All start out on.
enum pref_option {
	PREF_WRITE_TAGS = 0,			// write tags into the encoded files
	PREF_EMBED_COVER_ART,			// ... including the cover art
	PREF_TAG_MUSICBRAINZ_IDS,		// ... and the MusicBrainz ids
	PREF_WRITE_ATTRIBUTES,			// write the tags as BFS attributes
	PREF_ATTRIBUTE_MUSICBRAINZ_IDS,	// ... and the MusicBrainz ids
	PREF_SAVE_COVER_FILE,			// save cover.<ext> next to the tracks
	PREF_USE_MUSICBRAINZ,			// look up discs and cover art online
	PREF_FILL_YEAR,					// fill empty years from MusicBrainz

	PREF_OPTION_COUNT
};

class Settings : public BArchivable {
public:
	Settings();
	Settings(BMessage* archive);
	~Settings();

	virtual status_t Archive(BMessage* archive, bool deep = true) const;
	virtual BArchivable* Instantiate(BMessage* archive);

	static void OpenSettings();
	void SaveSettings();
/*
	BRect WindowFrame();
	void SetWindowFrame(BRect value);
*/
	bool IsEncoding();
	void SetEncoding(bool value);

	AEEncoder* Encoder();
	void SetEncoder(AEEncoder* value);

	const char* EncoderName();
	void SetEncoderName(const char* value);
	
	const char* PathPattern();
	void SetPathPattern(const char* value);

	const char* AddOnsDirectory();

	void SetColumnsState(BMessage* message);
	BMessage* ColumnsState();

	void SetLayoutState(BMessage* message);
	BMessage* LayoutState();

	bool Option(pref_option option) const;
	void SetOption(pref_option option, bool value);

	// The MIME type ("image/jpeg") cover.<ext> is saved as.
	const char* CoverFileMimeType() const;
	void SetCoverFileMimeType(const char* mimeType);

	// Genres picked in the field editor, newest first, for the top of its
	// genre menu.
	static const int32 kMaxRecentGenres = 8;
	const std::vector<BString>& RecentGenres() const;
	void AddRecentGenre(const char* genre);

	void PrintToStream();
private:
	void InitDefaults();
//	BRect windowFrame;
	bool encoding;
	AEEncoder* encoder;
	char encoderName[B_FILE_NAME_LENGTH];
	char pathPattern[B_PATH_NAME_LENGTH];
	char addonDirectory[B_PATH_NAME_LENGTH];
	BMessage columnsState;
	BMessage layoutState;
	bool options[PREF_OPTION_COUNT];
	BString coverFileMimeType;
	std::vector<BString> recentGenres;
};


extern Settings* settings;

#endif
