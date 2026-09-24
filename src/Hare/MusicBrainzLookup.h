/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __MUSIC_BRAINZ_LOOKUP_H__
#define __MUSIC_BRAINZ_LOOKUP_H__

#include <SupportDefs.h>

class BMessenger;
class BString;


class MusicBrainzLookup {
public:
	// deviceName is the cdda volume's raw device path, in the form
	// Haiku's fs_info::device_name already gives it (e.g.
	// "/dev/disk/atapi/1/master/raw") - exactly what libdiscid's Haiku
	// backend expects.
	//
	// fallbackQuery is a best-effort "Artist - Album" string (Hare passes
	// the cdda volume's own name) used to search MusicBrainz by title when
	// the disc's exact table of contents isn't registered there at all -
	// LookupDiscID() comes back empty for plenty of well known releases
	// simply because nobody has submitted that particular pressing's disc
	// ID. Pass NULL or "" when there's nothing usable to search with; the
	// exact disc ID lookup is always tried first regardless, and this is
	// only a fallback for when that comes back with nothing.
	static void FetchAsync(const char* deviceName, const char* fallbackQuery,
		BMessenger* replyTo);

private:
	static int32 FetchThread(void* args);
	static BString ComputeDiscID(const char* deviceName);
};

#endif
