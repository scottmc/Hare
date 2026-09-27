/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "MusicBrainzLookup.h"

#include <exception>
#include <new>
#include <string.h>
#include <vector>

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Debug.h>
#include <DataIO.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <String.h>
#include <TranslationDefs.h>
#include <TranslatorRoster.h>

#include <discid/discid.h>

#include <musicbrainz5/Query.h>
#include <musicbrainz5/Release.h>
#include <musicbrainz5/ReleaseList.h>
#include <musicbrainz5/ReleaseGroup.h>
#include <musicbrainz5/Medium.h>
#include <musicbrainz5/MediumList.h>
#include <musicbrainz5/Track.h>
#include <musicbrainz5/TrackList.h>
#include <musicbrainz5/Recording.h>
#include <musicbrainz5/ArtistCredit.h>
#include <musicbrainz5/NameCredit.h>
#include <musicbrainz5/NameCreditList.h>
#include <musicbrainz5/Artist.h>

#include <coverart/CoverArt.h>

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

// Decodes compressed image bytes (JPEG/PNG/whatever the Cover Art Archive
// handed back) into a BBitmap via Haiku's Translation Kit. Returns NULL on
// any failure. Caller owns the returned bitmap.
BBitmap*
DecodeImage(const unsigned char* data, size_t size)
{
	BMemoryIO memoryIO(data, size);

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (!roster) {
		return NULL;
	}

	BBitmapStream stream;
	status_t status = roster->Translate(&memoryIO, NULL, NULL, &stream,
		B_TRANSLATOR_BITMAP);
	if (status != B_OK) {
		PRINT(("MusicBrainzLookup: image decode failed: %s\n",
			strerror(status)));
		return NULL;
	}

	BBitmap* bitmap = NULL;
	stream.DetachBitmap(&bitmap);
	return bitmap;
}
// Joins a CArtistCredit's name-credit list into the single display string
// MusicBrainz itself uses for an artist credit, e.g. two collaborating
// artists become "Artist One feat. Artist Two" rather than either name
// alone - CNameCredit::JoinPhrase() is exactly the (often empty) separator
// text MusicBrainz stores between one credited name and the next.
BString
JoinArtistCredit(MusicBrainz5::CArtistCredit* credit)
{
	BString result;

	if (!credit || !credit->NameCreditList()) {
		return result;
	}

	MusicBrainz5::CNameCreditList* names = credit->NameCreditList();
	for (int i = 0; i < names->NumItems(); i++) {
		MusicBrainz5::CNameCredit* nameCredit = names->Item(i);
		if (!nameCredit) {
			continue;
		}
		result << nameCredit->Name().c_str();
		result << nameCredit->JoinPhrase().c_str();
	}

	return result;
}


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

BString
EscapeLuceneQuoted(const BString& text)
{
	BString result(text);
	result.ReplaceAll("\\", "\\\\");
	result.ReplaceAll("\"", "\\\"");
	return result;
}


// The first credited artist's MBID off a CArtistCredit - good enough for
// the single "artistId" attribute Hare writes; a release can have several
// credited artists (JoinArtistCredit() above covers the display string for
// all of them), but there's no widely-used convention for storing more
// than one artist MBID as a single file attribute.
BString
FirstArtistId(MusicBrainz5::CArtistCredit* credit)
{
	BString result;

	if (!credit || !credit->NameCreditList()
			|| (credit->NameCreditList()->NumItems() == 0)) {
		return result;
	}

	MusicBrainz5::CNameCredit* nameCredit = credit->NameCreditList()->Item(0);
	if (nameCredit && nameCredit->Artist()) {
		result = nameCredit->Artist()->ID().c_str();
	}

	return result;
}

// Sends METADATA_FOUND for release (matched against discId via
// CRelease::MediaMatchingDiscID(), so a multi-disc release resolves to the
// right medium/tracklist for the actual disc that was looked up). Best
// effort throughout - see MusicBrainzLookup.h's METADATA_FOUND
// documentation for the message's exact contents. Doesn't throw: any
// libmusicbrainz5 exception is caught and just means less metadata gets
// sent, same as a field simply being blank on the release itself.
void
SendMetadata(const BString& discId, MusicBrainz5::CRelease* release,
	BMessenger& replyTo)
{
	if (!release) {
		return;
	}

	BMessage msg(METADATA_FOUND);
	msg.AddString("discId", discId);
	msg.AddString("releaseId", release->ID().c_str());
	msg.AddString("albumTitle", release->Title().c_str());

	MusicBrainz5::CArtistCredit* albumCredit = release->ArtistCredit();
	BString albumArtist = JoinArtistCredit(albumCredit);
	msg.AddString("albumArtist", albumArtist);
	msg.AddString("artistId", FirstArtistId(albumCredit));

	MusicBrainz5::CReleaseGroup* releaseGroup = release->ReleaseGroup();
	if (releaseGroup) {
		msg.AddString("releaseGroupId", releaseGroup->ID().c_str());
	}

	BString date = release->Date().c_str();
	BString year;
	if (date.Length() >= 4) {
		date.CopyInto(year, 0, 4);
	}
	msg.AddString("year", year);

	try {
		MusicBrainz5::CMediumList media
			= release->MediaMatchingDiscID(discId.String());
		for (int m = 0; m < media.NumItems(); m++) {
			MusicBrainz5::CMedium* medium = media.Item(m);
			if (!medium || !medium->TrackList()) {
				continue;
			}

			MusicBrainz5::CTrackList* tracks = medium->TrackList();
			for (int t = 0; t < tracks->NumItems(); t++) {
				MusicBrainz5::CTrack* track = tracks->Item(t);
				if (!track) {
					continue;
				}

				MusicBrainz5::CRecording* recording = track->Recording();

				BString trackTitle = track->Title().c_str();
				if ((trackTitle.Length() == 0) && recording) {
					trackTitle = recording->Title().c_str();
				}

				MusicBrainz5::CArtistCredit* trackCredit
					= track->ArtistCredit();
				BString trackArtist = trackCredit
					? JoinArtistCredit(trackCredit) : BString();
				if (trackArtist.Length() == 0) {
					trackArtist = albumArtist;
				}

				BString recordingId;
				if (recording) {
					recordingId = recording->ID().c_str();
				}

				msg.AddInt32("trackNumber", track->Position());
				msg.AddString("trackTitle", trackTitle);
				msg.AddString("trackArtist", trackArtist);
				msg.AddString("recordingId", recordingId);
			}
		}
	} catch (std::exception& ex) {
		PRINT(("MusicBrainzLookup: reading track list failed: %s\n",
			ex.what()));
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
MusicBrainzLookup::ComputeDiscID(const char* deviceName)
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

	BString discId = ComputeDiscID(deviceName.String());
	if (discId.Length() == 0) {
		PRINT(("MusicBrainzLookup: could not compute a disc ID for %s\n",
			deviceName.String()));
		return B_ERROR;
	}

	PRINT(("MusicBrainzLookup: disc ID = %s\n", discId.String()));

	MusicBrainz5::CQuery query(MUSICBRAINZ_USER_AGENT);
	MusicBrainz5::CReleaseList candidateReleases;

	try {
		MusicBrainz5::CReleaseList releases = query.LookupDiscID(discId.String());

		if (releases.NumItems() > 0) {
			candidateReleases = releases;
		} else {
			PRINT(("MusicBrainzLookup: no releases found for disc ID %s\n",
				discId.String()));
		}

	} catch (std::exception& ex) {
		PRINT(("MusicBrainzLookup: disc ID lookup failed: %s\n", ex.what()));
	}

	// Kept alive for the rest of the function, same as candidateReleases
	// itself below - both may end up holding pointers into this that need
	// to outlive the try block they're captured in.
	MusicBrainz5::CMetadata searchMetadata;
	if (candidateReleases.NumItems() == 0) {
		BString fallbackArtist, fallbackAlbum;
		ParseArtistAlbum(fallbackQuery, fallbackArtist, fallbackAlbum);

		if ((fallbackArtist.Length() > 0) && (fallbackAlbum.Length() > 0)) {
			try {
				MusicBrainz5::CQuery::tParamMap searchParams;
				BString luceneQuery;
				luceneQuery << "artist:\"" << EscapeLuceneQuoted(fallbackArtist)
					<< "\" AND release:\"" << EscapeLuceneQuoted(fallbackAlbum)
					<< "\"";
				PRINT(("MusicBrainzLookup: falling back to title search: "
					"%s\n", luceneQuery.String()));
				searchParams["query"] = luceneQuery.String();

				searchMetadata = query.Query("release", "", "", searchParams);
				MusicBrainz5::CReleaseList* searchReleases
					= searchMetadata.ReleaseList();
				if (searchReleases && (searchReleases->NumItems() > 0)) {
					candidateReleases = *searchReleases;
				} else {
					PRINT(("MusicBrainzLookup: title search found no "
						"releases\n"));
				}
			} catch (std::exception& ex) {
				PRINT(("MusicBrainzLookup: title search failed: %s\n",
					ex.what()));
			}
		}
	}

	if (candidateReleases.NumItems() == 0) {
		return B_ERROR;
	}

	// Track/release metadata (title, artist, per-track recording IDs, ...)
	// always comes from the single best candidate, same as before this
	// changed to support multiple cover art candidates - only the cover
	// art itself is offered as a choice.
	MusicBrainz5::CRelease* release = candidateReleases.Item(0);
	BString releaseId = release->ID().c_str();
	SendMetadata(discId, release, replyTo);

	PRINT(("MusicBrainzLookup: release ID = %s\n", releaseId.String()));

	// One cover art fetch per candidate release, up to
	// MAX_COVER_ART_CANDIDATES - each is independent, so one candidate's
	// cover art being missing or failing to fetch doesn't stop the others
	// from being offered. "bitmap" and "imageData" are added to msg as
	// parallel indexed arrays (Haiku's usual BMessage idiom for this - see
	// METADATA_FOUND's per-track fields for another example), so a
	// candidate with no decodable bitmap still adds an explicit NULL
	// "bitmap" entry to keep the two arrays in step with each other.
	int32 candidateCount = candidateReleases.NumItems();
	if (candidateCount > MAX_COVER_ART_CANDIDATES) {
		candidateCount = MAX_COVER_ART_CANDIDATES;
	}

	BMessage msg(COVER_ART_FOUND);
	int32 foundCount = 0;
	for (int32 i = 0; i < candidateCount; i++) {
		MusicBrainz5::CRelease* candidate = candidateReleases.Item(i);
		if (!candidate) {
			continue;
		}
		BString candidateId = candidate->ID().c_str();

		std::vector<unsigned char> imageData;
		try {
			CoverArtArchive::CCoverArt coverArt(MUSICBRAINZ_USER_AGENT);
			imageData = coverArt.FetchFront(candidateId.String());
		} catch (std::exception& ex) {
			PRINT(("MusicBrainzLookup: cover art fetch failed for %s: "
				"%s\n", candidateId.String(), ex.what()));
			continue;
		}

		if (imageData.empty()) {
			PRINT(("MusicBrainzLookup: no cover art available for "
				"release %s\n", candidateId.String()));
			continue;
		}

		BBitmap* bitmap = DecodeImage(&imageData[0], imageData.size());
		msg.AddPointer("bitmap", bitmap);
		msg.AddData("imageData", B_RAW_TYPE, &imageData[0], imageData.size());
		foundCount++;
	}

	if (foundCount == 0) {
		PRINT(("MusicBrainzLookup: no cover art available for any "
			"candidate release\n"));
		return B_ERROR;
	}

	replyTo.SendMessage(&msg);

	return B_OK;
}
