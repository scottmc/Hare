/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __COVER_ART_VIEW_H__
#define __COVER_ART_VIEW_H__

#include <View.h>

class BBitmap;
class BRect;

class CoverArtView : public BView {
public:
	CoverArtView();
	virtual ~CoverArtView();
	virtual void Draw(BRect updateRect);
	virtual void FrameResized(float width, float height);
	virtual void MouseDown(BPoint where);
	virtual BSize MinSize();
	virtual BSize PreferredSize();
	virtual BSize MaxSize();
	void SetCoverArt(BBitmap* bitmap);

	// Lets AppView (which hears about CD mount/unmount from AppWindow)
	// tell us whether to show the "Load CD" button drawn over the
	// placeholder gradient below - see DrawLoadCdButton().
	void SetCdMounted(bool mounted);
private:
	BRect SquareBounds();
	void DrawPlaceholder(BRect square);
	void DrawLoadCdButton(BRect square);
	BRect LoadCdButtonRect(BRect square) const;
	BRect LoadCdIconRect(BRect buttonRect) const;
	BBitmap* coverArt;
	BBitmap* loadCdIcon;
	bool cdMounted;
};

#endif
