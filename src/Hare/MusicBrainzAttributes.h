/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __MUSIC_BRAINZ_ATTRIBUTES_H__
#define __MUSIC_BRAINZ_ATTRIBUTES_H__

#include <SupportDefs.h>

class BFile;
class AudioAttribute;

// Writes the MusicBrainz identifiers Hare looks up for a disc as Haiku
// file attributes, using the same AudioAttribute machinery (libHare's
// AudioAttributes/AudioAttribute) that already reads/writes the standard
// Audio:Artist/Audio:Album/etc. attributes - it self-registers each
// attribute's MIME attr-info the same way (AudioAttribute::Create()), so
// Tracker's Attributes menu shows these next to the standard ones.
//
// The attribute names match MusicBrainz Picard's own tag names
// (MUSICBRAINZ_DISCID/_ALBUMID/_TRACKID/_ARTISTID/_RELEASEGROUPID) under
// Hare's existing "Audio:" attribute prefix, both for consistency with
// Hare's own Audio:Artist/Audio:Album/etc. attributes and so the values
// read the same as what Picard itself would write into a file's tags -
// making it possible to re-identify a ripped track (or the disc it came
// from) later in Picard, ArmyKnife, or any other MusicBrainz-aware tool,
// without needing the physical disc again.
class MusicBrainzAttributes {
public:
	MusicBrainzAttributes(BFile* file);
	virtual ~MusicBrainzAttributes();

	virtual void SetDiscId(const char* value);
	virtual void SetReleaseId(const char* value);
	virtual void SetRecordingId(const char* value);
	virtual void SetArtistId(const char* value);
	virtual void SetReleaseGroupId(const char* value);

	// Writes whichever of the five values above were set (via their
	// Set*() call) since construction - a value never set is left alone
	// entirely, not written as blank, so re-encoding without a disc ID
	// available (MusicBrainz lookup gated off, or nothing found) doesn't
	// clobber attributes a previous run already wrote.
	virtual status_t Write();

private:
	status_t InitAttribute(AudioAttribute* attrib);

	BFile* file;
	AudioAttribute* discId;
	AudioAttribute* releaseId;
	AudioAttribute* recordingId;
	AudioAttribute* artistId;
	AudioAttribute* releaseGroupId;
};

#endif
