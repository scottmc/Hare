/*
 * Copyright 2000-2021, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CheckMark.h"

#include <Debug.h>

// BRect is inclusive of its right/bottom edges, so a kImageWidth x
// kImageHeight image is BRect(0, 0, kImageWidth - 1, kImageHeight - 1).
// Using (kImageWidth, kImageHeight) made a 16x16 bitmap: its extra right
// column showed the data's row padding byte (a vertical white line) and its
// extra bottom row was filled from past the end of kImageBits (garbage
// black/green line). The B_CMAP8 rows are padded to 16 bytes, so the
// 15x15 bitmap's bits length matches sizeof(kImageBits) exactly.
CheckMark::CheckMark()
	:
	BBitmap(BRect(0, 0, kImageWidth - 1, kImageHeight - 1), kImageColorSpace)
{
	int32 length = BitsLength();
	if (length > (int32)sizeof(kImageBits))
		length = sizeof(kImageBits);
	SetBits(kImageBits, length, 0, kImageColorSpace);
}
