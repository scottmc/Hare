/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CoverArtCandidatesView.h"

#include <Bitmap.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <Rect.h>


namespace {

const float kThumbnailSize = 96.0f;
const float kThumbnailSpacing = 8.0f;
const float kSelectionBorderWidth = 3.0f;
const float kEndPadding = 3.0f;

} // namespace


CoverArtCandidatesView::CoverArtCandidatesView()
	:
	BView("coverArtCandidatesView", B_WILL_DRAW | B_FRAME_EVENTS),
	fCandidates(NULL),
	fCount(0),
	fSelectedIndex(0)
{
	PRINT(("CoverArtCandidatesView::CoverArtCandidatesView()\n"));

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

CoverArtCandidatesView::~CoverArtCandidatesView()
{
	PRINT(("CoverArtCandidatesView::~CoverArtCandidatesView()\n"));

	for (int32 i = 0; i < fCount; i++) {
		delete fCandidates[i];
	}
	delete[] fCandidates;
}

void
CoverArtCandidatesView::SetCandidates(BBitmap* const* candidates, int32 count)
{
	PRINT(("CoverArtCandidatesView::SetCandidates(BBitmap* const*, "
		"int32)\n"));

	for (int32 i = 0; i < fCount; i++) {
		delete fCandidates[i];
	}
	delete[] fCandidates;
	fCandidates = NULL;
	fCount = 0;

	if (candidates && (count > 0)) {
		fCandidates = new BBitmap*[count];
		for (int32 i = 0; i < count; i++) {
			fCandidates[i] = candidates[i];
		}
		fCount = count;
	}

	fSelectedIndex = 0;

	ResizeTo(PreferredSize().Width(), PreferredSize().Height());
	InvalidateLayout();
	Invalidate();
}

void
CoverArtCandidatesView::Clear()
{
	PRINT(("CoverArtCandidatesView::Clear()\n"));

	SetCandidates(NULL, 0);
}

BSize
CoverArtCandidatesView::MinSize()
{
	return BSize(kThumbnailSize + (2 * kThumbnailSpacing) + (2 * kEndPadding),
		kThumbnailSize + (2 * kThumbnailSpacing));
}

BSize
CoverArtCandidatesView::PreferredSize()
{
	float width = (fCount * kThumbnailSize)
		+ ((fCount + 1) * kThumbnailSpacing) + (2 * kEndPadding);
	if (width < (kThumbnailSize + (2 * kThumbnailSpacing) + (2 * kEndPadding))) {
		width = kThumbnailSize + (2 * kThumbnailSpacing) + (2 * kEndPadding);
	}
	return BSize(width, kThumbnailSize + (2 * kThumbnailSpacing));
}

BSize
CoverArtCandidatesView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, kThumbnailSize + (2 * kThumbnailSpacing));
}

BRect
CoverArtCandidatesView::ThumbnailRect(int32 index) const
{
	float left = kEndPadding + kThumbnailSpacing
		+ (index * (kThumbnailSize + kThumbnailSpacing));
	float top = kThumbnailSpacing;
	return BRect(left, top, left + kThumbnailSize, top + kThumbnailSize);
}

void
CoverArtCandidatesView::Draw(BRect updateRect)
{
	for (int32 i = 0; i < fCount; i++) {
		BRect rect = ThumbnailRect(i);
		if (!rect.Intersects(updateRect)) {
			continue;
		}

		if (fCandidates[i]) {
			DrawBitmap(fCandidates[i], fCandidates[i]->Bounds(), rect);
		} else {
			SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			FillRect(rect);
			SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
			StrokeRect(rect);
		}

		if (i == fSelectedIndex) {
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			SetPenSize(kSelectionBorderWidth);
			StrokeRect(rect.InsetByCopy(-kSelectionBorderWidth / 2.0f,
				-kSelectionBorderWidth / 2.0f));
			SetPenSize(1.0f);
		}
	}
}

void
CoverArtCandidatesView::MouseDown(BPoint where)
{
	PRINT(("CoverArtCandidatesView::MouseDown(BPoint)\n"));

	for (int32 i = 0; i < fCount; i++) {
		if (ThumbnailRect(i).Contains(where)) {
			if (fSelectedIndex != i) {
				fSelectedIndex = i;
				Invalidate();
			}
			break;
		}
	}
}
