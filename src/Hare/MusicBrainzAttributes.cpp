/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "MusicBrainzAttributes.h"

#include <File.h>
#include <fs_attr.h>
#include <TypeConstants.h>

#include "AudioAttribute.h"

#define DISC_ID_NAME             "MusicBrainz Disc ID"
#define RELEASE_ID_NAME          "MusicBrainz Album ID"
#define RECORDING_ID_NAME        "MusicBrainz Track ID"
#define ARTIST_ID_NAME           "MusicBrainz Artist ID"
#define RELEASE_GROUP_ID_NAME    "MusicBrainz Release Group ID"

// These match MusicBrainz Picard's own tag names (see MusicBrainzLookup.h/
// MusicBrainzAttributes.h) under Hare's existing "Audio:" attribute prefix.
#define DISC_ID_ATTR             "Audio:MUSICBRAINZ_DISCID"
#define RELEASE_ID_ATTR          "Audio:MUSICBRAINZ_ALBUMID"
#define RECORDING_ID_ATTR        "Audio:MUSICBRAINZ_TRACKID"
#define ARTIST_ID_ATTR           "Audio:MUSICBRAINZ_ARTISTID"
#define RELEASE_GROUP_ID_ATTR    "Audio:MUSICBRAINZ_RELEASEGROUPID"

MusicBrainzAttributes::MusicBrainzAttributes(BFile* file)
{
	this->file = file;
	discId = new AudioAttribute(file, DISC_ID_NAME, DISC_ID_ATTR,
		B_STRING_TYPE);
	releaseId = new AudioAttribute(file, RELEASE_ID_NAME, RELEASE_ID_ATTR,
		B_STRING_TYPE);
	recordingId = new AudioAttribute(file, RECORDING_ID_NAME,
		RECORDING_ID_ATTR, B_STRING_TYPE);
	artistId = new AudioAttribute(file, ARTIST_ID_NAME, ARTIST_ID_ATTR,
		B_STRING_TYPE);
	releaseGroupId = new AudioAttribute(file, RELEASE_GROUP_ID_NAME,
		RELEASE_GROUP_ID_ATTR, B_STRING_TYPE);

	InitAttribute(discId);
	InitAttribute(releaseId);
	InitAttribute(recordingId);
	InitAttribute(artistId);
	InitAttribute(releaseGroupId);
}

MusicBrainzAttributes::~MusicBrainzAttributes()
{
	delete discId;
	delete releaseId;
	delete recordingId;
	delete artistId;
	delete releaseGroupId;
}

status_t
MusicBrainzAttributes::InitAttribute(AudioAttribute* attrib)
{
	if (!attrib) {
		return B_ERROR;
	}

	if (!attrib->Exists()) {
		return attrib->Create();
	}

	return B_OK;
}

void
MusicBrainzAttributes::SetDiscId(const char* value)
{
	discId->SetValue(value);
}

void
MusicBrainzAttributes::SetReleaseId(const char* value)
{
	releaseId->SetValue(value);
}

void
MusicBrainzAttributes::SetRecordingId(const char* value)
{
	recordingId->SetValue(value);
}

void
MusicBrainzAttributes::SetArtistId(const char* value)
{
	artistId->SetValue(value);
}

void
MusicBrainzAttributes::SetReleaseGroupId(const char* value)
{
	releaseGroupId->SetValue(value);
}

status_t
MusicBrainzAttributes::Write()
{
	// Only ever write an attribute that was actually given a non-empty
	// value via its Set*() call above - AudioAttribute::Value() is NULL
	// both when Set*() was never called and when it was called with NULL/
	// "", so either way there's nothing meaningful to write, and skipping
	// it entirely (rather than writing a blank attribute) means
	// re-encoding without a disc ID available doesn't clobber whatever an
	// earlier run already wrote.
	status_t result = B_OK;

	if (discId->Value() && (discId->Write() != B_OK)) {
		result = B_ERROR;
	}
	if (releaseId->Value() && (releaseId->Write() != B_OK)) {
		result = B_ERROR;
	}
	if (recordingId->Value() && (recordingId->Write() != B_OK)) {
		result = B_ERROR;
	}
	if (artistId->Value() && (artistId->Write() != B_OK)) {
		result = B_ERROR;
	}
	if (releaseGroupId->Value() && (releaseGroupId->Write() != B_OK)) {
		result = B_ERROR;
	}

	return result;
}
