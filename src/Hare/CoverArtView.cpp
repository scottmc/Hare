/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CoverArtView.h"

#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <Debug.h>
#include <File.h>
#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <NodeInfo.h>
#include <math.h>
#include <Rect.h>
#include <Region.h>
#include <Roster.h>
#include <String.h>
#include <Window.h>

#include "CommandConstants.h"


namespace {

// The icon+label "Load CD" button drawn over the placeholder gradient
// (see DrawLoadCdButton()) when a CD is mounted but no cover art has
// been picked yet.
const float kLoadCdIconSize = 32.0f;
const float kLoadCdLabelGap = 4.0f;
const float kLoadCdButtonPadding = 8.0f;
const char* kLoadCdButtonLabel = "Load CD";

} // namespace


CoverArtView::CoverArtView()
	:
	BView("coverArtView", B_WILL_DRAW | B_FRAME_EVENTS
		| B_FULL_UPDATE_ON_RESIZE),
	coverArt(0),
	loadCdIcon(NULL),
	cdMounted(false)
{
	PRINT(("CoverArtView::CoverArtView()\n"));

	SetViewColor(B_TRANSPARENT_COLOR);

	// Grab Hare's own HVIF app icon (the same one Tracker/Deskbar show)
	// to draw on the "Load CD" button rather than bundling a second
	// copy of the artwork just for this.
	app_info info;
	if (be_app->GetAppInfo(&info) == B_OK) {
		BFile appFile(&info.ref, B_READ_ONLY);
		BNodeInfo nodeInfo(&appFile);
		loadCdIcon = new BBitmap(BRect(0, 0, 31, 31), B_RGBA32);
		if (nodeInfo.GetTrackerIcon(loadCdIcon, B_LARGE_ICON) != B_OK) {
			delete loadCdIcon;
			loadCdIcon = NULL;
		}
	}
}

CoverArtView::~CoverArtView()
{
	PRINT(("CoverArtView::~CoverArtView()\n"));

	delete coverArt;
	delete loadCdIcon;
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
CoverArtView::MouseDown(BPoint where)
{
	PRINT(("CoverArtView::MouseDown(BPoint)\n"));

	// The "Load CD" button only exists (and is only drawn) in the
	// placeholder state: no cover art loaded yet, and a CD mounted for
	// it to load. See Draw().
	if (coverArt || !cdMounted) {
		return;
	}

	if (LoadCdButtonRect(SquareBounds()).Contains(where)) {
		Window()->PostMessage(new BMessage(LOAD_CD_BUTTON_PRESSED));
	}
}

void
CoverArtView::SetCoverArt(BBitmap* bitmap)
{
	PRINT(("CoverArtView::SetCoverArt(BBitmap*)\n"));

	delete coverArt;
	coverArt = bitmap;
	Invalidate();
}

void
CoverArtView::SetCdMounted(bool mounted)
{
	PRINT(("CoverArtView::SetCdMounted(bool)\n"));

	if (cdMounted != mounted) {
		cdMounted = mounted;
		Invalidate();
	}
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
		if (cdMounted) {
			DrawLoadCdButton(square);
		}
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

void
CoverArtView::DrawLoadCdButton(BRect square)
{
	BRect buttonRect = LoadCdButtonRect(square);

	// A translucent white chip behind the icon and label keeps them
	// readable regardless of where the gradient background happens to
	// sit under this particular button (dark blue up top, pale blue
	// down at the bottom).
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	SetHighColor(255, 255, 255, 160);
	FillRoundRect(buttonRect, 8.0f, 8.0f);

	BRect iconRect = LoadCdIconRect(buttonRect);
	if (loadCdIcon) {
		DrawBitmap(loadCdIcon, loadCdIcon->Bounds(), iconRect);
	}
	SetDrawingMode(B_OP_COPY);

	BString label(kLoadCdButtonLabel);
	float labelWidth = StringWidth(label.String());
	font_height fh;
	GetFontHeight(&fh);

	BPoint labelPoint(
		buttonRect.left + ((buttonRect.Width() - labelWidth) / 2.0f),
		iconRect.bottom + kLoadCdLabelGap + fh.ascent);

	SetHighColor(40, 40, 40, 255);
	DrawString(label.String(), labelPoint);
}

BRect
CoverArtView::LoadCdButtonRect(BRect square) const
{
	font_height fh;
	GetFontHeight(&fh);
	float labelHeight = ceilf(fh.ascent + fh.descent + fh.leading);

	float labelWidth = StringWidth(kLoadCdButtonLabel);
	float blockWidth = (labelWidth > kLoadCdIconSize)
		? labelWidth : kLoadCdIconSize;
	float blockHeight = kLoadCdIconSize + kLoadCdLabelGap + labelHeight;

	float width = blockWidth + (2 * kLoadCdButtonPadding);
	float height = blockHeight + (2 * kLoadCdButtonPadding);

	BRect rect(0, 0, width, height);
	rect.OffsetBy(square.left + ((square.Width() - width) / 2.0f),
		square.top + ((square.Height() - height) / 2.0f));
	return rect;
}

BRect
CoverArtView::LoadCdIconRect(BRect buttonRect) const
{
	float left = buttonRect.left
		+ ((buttonRect.Width() - kLoadCdIconSize) / 2.0f);
	float top = buttonRect.top + kLoadCdButtonPadding;
	return BRect(left, top, left + kLoadCdIconSize, top + kLoadCdIconSize);
}
