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
	virtual BSize MinSize();
	virtual BSize PreferredSize();
	virtual BSize MaxSize();
	void SetCoverArt(BBitmap* bitmap);
private:
	BRect SquareBounds();
	void DrawPlaceholder(BRect square);
	BBitmap* coverArt;
};

#endif
