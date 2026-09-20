/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CoverArtView.h"

#include <Bitmap.h>
#include <Debug.h>
#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <Region.h>


CoverArtView::CoverArtView()
	:
	BView("coverArtView", B_WILL_DRAW | B_FRAME_EVENTS
		| B_FULL_UPDATE_ON_RESIZE),
	coverArt(0)
{
	PRINT(("CoverArtView::CoverArtView()\n"));

	SetViewColor(B_TRANSPARENT_COLOR);
}

CoverArtView::~CoverArtView()
{
	PRINT(("CoverArtView::~CoverArtView()\n"));

	delete coverArt;
}

BSize
CoverArtView::MinSize()
{
	return BSize(64.0f, 64.0f);
}

BSize
CoverArtView::PreferredSize()
{
	return BSize(150.0f, 150.0f);
}

BSize
CoverArtView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}

void
CoverArtView::FrameResized(float width, float height)
{
	PRINT(("CoverArtView::FrameResized(float,float)\n"));

	BView::FrameResized(width, height);

	// The square we draw into depends on our current bounds, so a resize
	// (from either splitter) always needs a full redraw.
	Invalidate();
}

void
CoverArtView::SetCoverArt(BBitmap* bitmap)
{
	PRINT(("CoverArtView::SetCoverArt(BBitmap*)\n"));

	delete coverArt;
	coverArt = bitmap;
	Invalidate();
}

BRect
CoverArtView::SquareBounds()
{
	BRect bounds = Bounds();
	float side = bounds.Width() < bounds.Height()
		? bounds.Width() : bounds.Height();

	BRect square(0, 0, side, side);
	square.OffsetBy((bounds.Width() - side) / 2.0f,
		(bounds.Height() - side) / 2.0f);

	return square;
}

void
CoverArtView::Draw(BRect updateRect)
{
	BRect bounds = Bounds();
	BRect square = SquareBounds();

	// Letterbox any leftover margin (when the splitters leave us a
	// non-square area) with the panel background so the square doesn't
	// look like it's floating in stray color.
	if (square != bounds) {
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		BRegion margin(bounds);
		margin.Exclude(square);
		FillRegion(&margin);
	}

	if (coverArt) {
		DrawBitmap(coverArt, coverArt->Bounds(), square);
	} else {
		DrawPlaceholder(square);
	}
}

void
CoverArtView::DrawPlaceholder(BRect square)
{
	BGradientLinear gradient;
	gradient.AddColor((rgb_color){ 40, 70, 160, 255 }, 0);
	gradient.AddColor((rgb_color){ 150, 200, 250, 255 }, 255);
	gradient.SetStart(square.LeftTop());
	gradient.SetEnd(square.LeftBottom());

	FillRect(square, gradient);
}
