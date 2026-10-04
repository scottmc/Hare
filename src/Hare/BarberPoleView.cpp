/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * The animated diagonal-stripe drawing here is adapted from ArmyKnife's
 * Barberpole class (source/barberpole.cpp/.h at
 * github.com/HaikuArchives/ArmyKnife), released into the public domain by
 * its author, jonas.sundstrom@kirilla.com.
 */
#include "BarberPoleView.h"

#include <Debug.h>
#include <Font.h>
#include <InterfaceDefs.h>
#include <Window.h>
#include <math.h>

// Same pulse rate ArmyKnife's Barberpole uses while animating - a plain
// BView has no other B_PULSE_NEEDED user in Hare (grepped for it), so
// changing the window's rate here doesn't step on anything else.
static const bigtime_t kBarberPolePulseRate = 100000; // 100 ms
static const bigtime_t kIdlePulseRate = 500000; // Window()'s usual default


BarberPoleView::BarberPoleView(const char* name, uint32 flags)
	:
	BView(name, flags),
	fIsRunning(false)
{
	// The classic 8x8 diagonal-stripe bit pattern - each Pulse() rotates
	// it by one bit to animate the stripes scrolling.
	fPattern.data[0] = 0x0f;
	fPattern.data[1] = 0x1e;
	fPattern.data[2] = 0x3c;
	fPattern.data[3] = 0x78;
	fPattern.data[4] = 0xf0;
	fPattern.data[5] = 0xe1;
	fPattern.data[6] = 0xc3;
	fPattern.data[7] = 0x87;

	SetFont(be_plain_font);
}


BarberPoleView::~BarberPoleView()
{
}


void
BarberPoleView::Start(const char* label)
{
	PRINT(("BarberPoleView::Start(const char*)\n"));

	fLabel = label;
	fIsRunning = true;
	if (Window()) {
		Window()->SetPulseRate(kBarberPolePulseRate);
	}
	SetFlags(Flags() | B_PULSE_NEEDED);
	Invalidate();
}


void
BarberPoleView::Stop()
{
	PRINT(("BarberPoleView::Stop()\n"));

	fIsRunning = false;
	fLabel = "";
	if (Window()) {
		Window()->SetPulseRate(kIdlePulseRate);
	}
	SetFlags(Flags() & ~B_PULSE_NEEDED);
	Invalidate();
}


void
BarberPoleView::Pulse()
{
	if (!fIsRunning) {
		return;
	}

	uchar last = fPattern.data[7];
	for (int i = 7; i > 0; i--) {
		fPattern.data[i] = fPattern.data[i - 1];
	}
	fPattern.data[0] = last;

	Invalidate();
}


void
BarberPoleView::Draw(BRect updateRect)
{
	// Mirrors BStatusBar::Draw()'s own geometry (see Haiku's
	// src/kits/interface/StatusBar.cpp) so this view lines up with
	// statusBar - which it's swapped in for, in the same layout slot -
	// close enough that the label text doesn't visibly jump around when
	// the two are switched.
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float barTop = ceilf(fontHeight.ascent + fontHeight.descent) + 6;
	float baseLine = ceilf(fontHeight.ascent) + 1;

	BRect bounds = Bounds();

	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(BRect(bounds.left, bounds.top, bounds.right, barTop - 1));

	if (fLabel.Length() > 0) {
		SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		BFont font;
		GetFont(&font);
		BString label(fLabel);
		font.TruncateString(&label, B_TRUNCATE_END, bounds.Width());
		DrawString(label.String(), BPoint(bounds.left, baseLine));
	}

	BRect barRect(bounds.left, barTop, bounds.right, bounds.bottom);
	if (!fIsRunning) {
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		FillRect(barRect);
		return;
	}

	SetHighColor(ui_color(B_STATUS_BAR_COLOR));
	SetDrawingMode(B_OP_COPY);
	FillRect(barRect, fPattern);

	// A plain single-pixel frame around the bar area, echoing the light
	// border be_control_look draws around a real BStatusBar's bar.
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_2_TINT));
	SetDrawingMode(B_OP_OVER);
	StrokeRect(barRect);
	SetDrawingMode(B_OP_COPY);
}


void
BarberPoleView::FrameResized(float width, float height)
{
	Invalidate();
}
