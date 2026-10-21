/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "MusicBrainzLookup.h"

#include <new>
#include <string.h>
#include <vector>

#include <Bitmap.h>
#include <Debug.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <String.h>

#include <discid/discid.h>

#include "tagkit/CoverArtFetch.h"
#include "tagkit/CoverArtImage.h"
#include "tagkit/MusicBrainzSearch.h"
#include "tagkit/ReleaseInfo.h"

#include "AppDefs.h"
#include "CommandConstants.h"

namespace {

// Bundles up FetchAsync()'s arguments to hand across to the spawned
// thread - spawn_thread() only takes a single void* payload.
struct FetchParams {
	BString deviceName;
	BString fallbackQuery;
	BMessenger replyTo;
};

// MAX_COVER_ART_CANDIDATES (CommandConstants.h) caps how many candidate
// releases get their cover art fetched below and offered to the user to
// choose between (see COVER_ART_FOUND below and CoverArtCandidatesView) -
// almost always just one release, but the fallback title search below can
// turn up several plausible matches with no good way to tell which is
// right from metadata alone, and even an exact disc ID can legitimately
// match more than one release (different regional pressings sharing the
// same table of contents).

void
ParseArtistAlbum(const BString& fallbackQuery, BString& artist, BString& album)
{
	int32 index = fallbackQuery.FindFirst(" - ");
	if (index < 0) {
		return;
	}

	artist.SetTo(fallbackQuery, index);
	album = fallbackQuery;
	album.Remove(0, index + 3);
	artist.Trim();
	album.Trim();
}

// Sends METADATA_FOUND for release. withTracks is false for a release
// found by title search whose track list doesn't clearly fit the disc in
// the drive (see MusicBrainzSearch::LookupDisc()), so only the album-level
// details are sent then. Best effort throughout - see
// MusicBrainzLookup.h's METADATA_FOUND documentation for the message's
// exact contents.
void
SendMetadata(const BString& discId, const tagkit::ReleaseInfo& release,
	bool withTracks, BMessenger& replyTo)
{
	BMessage msg(METADATA_FOUND);
	msg.AddString("discId", discId);
	msg.AddString("releaseId", release.id);
	msg.AddString("albumTitle", release.title);
	msg.AddString("albumArtist", release.artist);
	msg.AddString("artistId", release.artistId);

	if (release.releaseGroupId.Length() > 0)
		msg.AddString("releaseGroupId", release.releaseGroupId);

	BString year;
	if (release.year > 0)
		year << release.year;
	msg.AddString("year", year);

	if (withTracks) {
		for (size_t t = 0; t < release.tracks.size(); t++) {
			const tagkit::TrackInfo& track = release.tracks[t];
			msg.AddInt32("trackNumber", track.number);
			msg.AddString("trackTitle", track.title);
			msg.AddString("trackArtist", track.artist.Length() > 0
				? track.artist : release.artist);
			msg.AddString("recordingId", track.recordingId);
		}
	}

	replyTo.SendMessage(&msg);
}

// Sends MUSICBRAINZ_LOOKUP_FINISHED from its destructor, so
// FetchThread() is guaranteed to signal completion - success or any
// of its several failure returns alike - no matter which return
// statement actually gets taken. AppView greys out Encode between the
// matching LOOKUP_STARTED (sent by FetchAsync(), right before it
// spawns the thread) and this, so an encode run can't start out from
// under a still-in-flight lookup and silently miss the cover art/
// metadata it hasn't received yet.
class LookupFinishedNotifier {
public:
	LookupFinishedNotifier(BMessenger replyTo)
		:
		fReplyTo(replyTo)
	{
	}

	~LookupFinishedNotifier()
	{
		BMessage msg(MUSICBRAINZ_LOOKUP_FINISHED);
		fReplyTo.SendMessage(&msg);
	}

private:
	BMessenger fReplyTo;
};

// Longest ComputeDiscID() (below) lets discid_read_sparse() block before
// giving up on it, mirroring TimedRead()'s CD_READ_TIMEOUT_SECONDS in
// AppView.cpp - libdiscid's Haiku backend issues its own raw SCSI/ATAPI
// commands to read the disc's table of contents and has no timeout of
// its own, so a drive that's stopped answering (ejected, disconnected,
// or - as actually seen - stuck retrying a failed DMA transfer) can
// leave this call blocked forever. That would leave FetchThread() (and
// so the MUSICBRAINZ_LOOKUP_STARTED/FINISHED pair, and the Encode
// button it gates) stuck right along with it.
#define DISC_ID_READ_TIMEOUT_SECONDS 10
const bigtime_t kDiscIdReadTimeout = DISC_ID_READ_TIMEOUT_SECONDS * 1000000LL;

// Sentinel TimedDiscIdReadSparse() (below) returns on a timeout - distinct
// from the real discid_read_sparse()'s own 0 (failure) and 1 (success),
// same idea as kCdReadTimedOut in AppView.cpp.
const int kDiscIdReadTimedOut = -1;

struct DiscIdReadArgs {
	DiscId* disc;
	BString deviceName;
	int result;
	sem_id done;
};

int32
DiscIdReadThread(void* cookie)
{
	DiscIdReadArgs* args = (DiscIdReadArgs*)cookie;
	args->result = discid_read_sparse(args->disc, args->deviceName.String(),
		0);
	release_sem(args->done);
	return B_OK;
}

// Wraps a single discid_read_sparse() call with a hard wall-clock
// timeout, the same way TimedRead() (AppView.cpp) wraps BFile::Read().
// The read itself runs on a short-lived helper thread; if it hasn't
// finished within timeoutMicros, this gives up on it and returns
// kDiscIdReadTimedOut instead of waiting any longer.
//
// On a timeout, the helper thread is killed rather than joined, so it
// can still be mid-ioctl against 'disc' at the moment it dies -
// callers MUST treat kDiscIdReadTimedOut as "leaked, don't touch 'disc'
// again" (no discid_get_error_msg(), no discid_free()) rather than
// freeing it, for the same reason TimedRead() leaks its buffer on
// timeout instead of freeing it: that would trade a hang for a
// use-after-free.
int
TimedDiscIdReadSparse(DiscId* disc, const char* deviceName,
	bigtime_t timeoutMicros)
{
	DiscIdReadArgs* args = new (std::nothrow) DiscIdReadArgs();
	if (!args) {
		// Couldn't even set up the timeout machinery - fall back to a
		// plain, untimed read rather than failing the lookup over it.
		return discid_read_sparse(disc, deviceName, 0);
	}
	args->disc = disc;
	args->deviceName = deviceName;
	args->result = 0;
	args->done = create_sem(0, "discid_read_done");
	if (args->done < B_OK) {
		int result = discid_read_sparse(disc, deviceName, 0);
		delete args;
		return result;
	}

	thread_id reader = spawn_thread(DiscIdReadThread, "_DiscIdRead_",
		B_LOW_PRIORITY, args);
	if (reader < B_OK) {
		delete_sem(args->done);
		int result = discid_read_sparse(disc, deviceName, 0);
		delete args;
		return result;
	}
	resume_thread(reader);

	status_t waitStatus = acquire_sem_etc(args->done, 1, B_RELATIVE_TIMEOUT,
		timeoutMicros);
	if (waitStatus != B_OK) {
		kill_thread(reader);
		return kDiscIdReadTimedOut;
	}

	wait_for_thread(reader, NULL);
	int result = args->result;
	delete_sem(args->done);
	delete args;
	return result;
}

} // namespace

void
MusicBrainzLookup::FetchAsync(const char* deviceName, const char* fallbackQuery,
	BMessenger* replyTo)
 {
	PRINT(("MusicBrainzLookup::FetchAsync(const char*, const char*, "
		"BMessenger*)\n"));

 	if (!deviceName || !replyTo) {
 		return;
 	}

	FetchParams* params = new (std::nothrow) FetchParams();
	if (!params) {
		return;
	}
 	params->deviceName = deviceName;
	params->fallbackQuery = fallbackQuery ? fallbackQuery : "";
 	params->replyTo = *replyTo;

	// Signalled here, right before the lookup actually starts, so AppView
	// can grey out Encode for its duration - matched by
	// LookupFinishedNotifier inside FetchThread (success or failure
	// alike), or right below if the thread never even gets spawned.
	BMessage startedMsg(MUSICBRAINZ_LOOKUP_STARTED);
	replyTo->SendMessage(&startedMsg);

	thread_id thread = spawn_thread(MusicBrainzLookup::FetchThread,
		"_MusicBrainzLookup_", B_LOW_PRIORITY, (void*)params);
	if (thread < B_OK) {
		delete params;
		BMessage finishedMsg(MUSICBRAINZ_LOOKUP_FINISHED);
		replyTo->SendMessage(&finishedMsg);
		return;
	}
	resume_thread(thread);
}

BString
MusicBrainzLookup::ComputeDiscID(const char* deviceName, int32* trackCount)
{
	PRINT(("MusicBrainzLookup::ComputeDiscID(const char*)\n"));

	BString result;

	DiscId* disc = discid_new();
	if (!disc) {
		return result;
	}

	int readResult = TimedDiscIdReadSparse(disc, deviceName,
		kDiscIdReadTimeout);
	if (readResult == kDiscIdReadTimedOut) {
		// See TimedDiscIdReadSparse()'s own comment - 'disc' is
		// deliberately leaked here rather than freed, since the helper
		// thread that timed out could still be using it.
		PRINT(("MusicBrainzLookup: discid_read_sparse(\"%s\") timed out "
			"after %d seconds\n", deviceName, DISC_ID_READ_TIMEOUT_SECONDS));
		return result;
	}

	if (readResult == 0) {
		PRINT(("MusicBrainzLookup: discid_read_sparse(\"%s\") failed: %s\n",
			deviceName, discid_get_error_msg(disc)));
		discid_free(disc);
		return result;
	}

	const char* id = discid_get_id(disc);
	if (id) {
		result = id;
	}

	if (trackCount != NULL) {
		*trackCount = discid_get_last_track_num(disc)
			- discid_get_first_track_num(disc) + 1;
	}

	discid_free(disc);
	return result;
}


int32
MusicBrainzLookup::FetchThread(void* args)
{
	PRINT(("MusicBrainzLookup::FetchThread(void*)\n"));

	FetchParams* params = (FetchParams*)args;
	BString deviceName = params->deviceName;
	BString fallbackQuery = params->fallbackQuery;
	BMessenger replyTo = params->replyTo;
	delete params;

	LookupFinishedNotifier lookupFinished(replyTo);

	int32 trackCount = 0;
	BString discId = ComputeDiscID(deviceName.String(), &trackCount);
	if (discId.Length() == 0) {
		PRINT(("MusicBrainzLookup: could not compute a disc ID for %s\n",
			deviceName.String()));
		return B_ERROR;
	}

	PRINT(("MusicBrainzLookup: disc ID = %s\n", discId.String()));

	// By disc ID if MusicBrainz knows the disc; if not, by the artist and
	// album the volume name gives (see FetchAsync()'s fallbackQuery) - a
	// release found that way lends its track list only when it clearly
	// fits the disc. Track/release metadata (title, artist, per-track
	// recording IDs, ...) always comes from the single best release; only
	// the cover art is offered as a choice between several.
	BString fallbackArtist, fallbackAlbum;
	ParseArtistAlbum(fallbackQuery, fallbackArtist, fallbackAlbum);
	if ((fallbackArtist.Length() == 0) || (fallbackAlbum.Length() == 0)) {
		fallbackArtist = "";
		fallbackAlbum = "";
	}

	tagkit::DiscLookupResult found = tagkit::MusicBrainzSearch::LookupDisc(
		discId, trackCount, fallbackArtist, fallbackAlbum,
		MAX_COVER_ART_CANDIDATES);
	if (found.status != B_OK) {
		PRINT(("MusicBrainzLookup: no release found for disc ID %s (%s)\n",
			discId.String(), strerror(found.status)));
		return B_ERROR;
	}

	SendMetadata(discId, found.release, found.hasTracks, replyTo);

	PRINT(("MusicBrainzLookup: release ID = %s (%s)\n",
		found.release.id.String(),
		found.foundByDiscId ? "by disc ID" : "by title search"));

	std::vector<tagkit::ReleaseRef> candidates = found.candidates;

	// One cover art fetch per candidate release, up to
	// MAX_COVER_ART_CANDIDATES - each is independent, so one candidate's
	// cover art being missing or failing to fetch doesn't stop the others
	// from being offered. "bitmap" and "imageData" are added to msg as
	// parallel indexed arrays (Haiku's usual BMessage idiom for this - see
	// METADATA_FOUND's per-track fields for another example), so a
	// candidate with no decodable bitmap still adds an explicit NULL
	// "bitmap" entry to keep the two arrays in step with each other.
	if (candidates.size() > (size_t)MAX_COVER_ART_CANDIDATES)
		candidates.resize(MAX_COVER_ART_CANDIDATES);

	std::vector<tagkit::CoverArtImage> images
		= tagkit::fetch_cover_art(candidates, MAX_COVER_ART_CANDIDATES);
	if (images.empty()) {
		PRINT(("MusicBrainzLookup: no cover art available for any "
			"candidate release\n"));
		return B_ERROR;
	}

	BMessage msg(COVER_ART_FOUND);
	for (size_t i = 0; i < images.size(); i++) {
		const tagkit::CoverArtImage& image = images[i];
		if (image.data.empty())
			continue;
		BBitmap* bitmap = tagkit::decode_cover_art(image);
		msg.AddPointer("bitmap", bitmap);
		msg.AddData("imageData", B_RAW_TYPE, &image.data[0],
			image.data.size());
	}

	replyTo.SendMessage(&msg);

	return B_OK;
}
