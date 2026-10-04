/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * The animated diagonal-stripe drawing here is adapted from ArmyKnife's
 * Barberpole class (source/barberpole.cpp/.h at
 * github.com/HaikuArchives/ArmyKnife), released into the public domain by
 * its author, jonas.sundstrom@kirilla.com.
 */
#ifndef __BARBER_POLE_VIEW_H__
#define __BARBER_POLE_VIEW_H__

#include <String.h>
#include <View.h>

// A classic BeOS/Haiku "barber pole" indeterminate-progress indicator:
// animated diagonal stripes, for a background task where there's no
// percentage to report (MusicBrainzLookup's disc ID/metadata/cover art
// fetch, currently the only user - see AppView::UpdateStatusCard()).
//
// Laid out as one of two cards (see AppView::InitView()'s statusBoxView)
// sharing the same slot statusBar occupies, so it takes over that same
// area of the window - and, to look like it belongs there, draws its own
// label text in the same spot BStatusBar draws its own Label()/Text(): a
// single left-aligned line above the bar area (see Haiku's
// BStatusBar::Draw() - the geometry here is deliberately kept in step
// with it).
class BarberPoleView : public BView {
public:
							BarberPoleView(const char* name, uint32 flags);
	virtual					~BarberPoleView();

	// Starts/stops the animation (via the window's Pulse() mechanism) and
	// shows/clears the label. AppView::UpdateStatusCard() is the only
	// caller - it also handles swapping this view's card into view.
			void			Start(const char* label);
			void			Stop();

	virtual	void			Pulse();
	virtual	void			Draw(BRect updateRect);
	virtual	void			FrameResized(float width, float height);

private:
			bool			fIsRunning;
			BString			fLabel;
			pattern			fPattern;
};

#endif	// __BARBER_POLE_VIEW_H__
