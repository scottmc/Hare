/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __COVER_ART_CANDIDATES_VIEW_H__
#define __COVER_ART_CANDIDATES_VIEW_H__

#include <View.h>

class BBitmap;

// Shows a horizontally scrollable row of MusicBrainz cover-art candidates -
// used instead of the single-image CoverArtView (see AppView.cpp's
// InitView() and its BCardLayout) whenever the disc's MusicBrainz lookup
// matched more than one plausible release and fetched each one's cover
// art. Clicking a thumbnail marks it as selected with a bold border; the
// selection defaults to the first candidate and is read by
// AppView::EncodeThread() (via SelectedIndex()) when Encode is pressed, so
// changing the selection after that point has no effect on a run already
// under way.
//
// Meant to live inside a horizontal-only BScrollView: this view reports
// its own natural width via PreferredSize(), sized to fit every candidate
// in a single row, so the scroll view knows how far there is to scroll.
class CoverArtCandidatesView : public BView {
public:
	CoverArtCandidatesView();
	virtual ~CoverArtCandidatesView();

	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint where);
	virtual BSize MinSize();
	virtual BSize PreferredSize();
	virtual BSize MaxSize();

	// Takes ownership of every non-NULL bitmap in candidates (matching
	// CoverArtView::SetCoverArt()'s own convention) - replaces whatever
	// set of candidates was previously shown, freeing those, and resets
	// the selection back to the first candidate. A NULL entry stands for
	// a candidate whose compressed bytes didn't decode into a bitmap; it
	// still gets a slot (drawn as a placeholder square) rather than being
	// skipped, so the position the user clicks on keeps lining up with
	// AppView's own parallel list of candidates' raw image bytes.
	void SetCandidates(BBitmap* const* candidates, int32 count);
	void Clear();

	int32 SelectedIndex() const { return fSelectedIndex; }
	int32 CountCandidates() const { return fCount; }

private:
	BRect ThumbnailRect(int32 index) const;

	BBitmap** fCandidates;
	int32 fCount;
	int32 fSelectedIndex;
};

#endif
