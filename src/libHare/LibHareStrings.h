/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __LIB_HARE_STRINGS_H__
#define __LIB_HARE_STRINGS_H__

#include <Catalog.h>

// libHare is a separate library from the Hare app itself (also linked by
// the encoder add-ons), so its user-visible strings get their own header
// here rather than sharing the app's GUIStrings.h.

// AudioAttributes.cpp: human-readable attribute names, written out as
// each file attribute's attr:public_name - this is what Tracker (and
// Haiku's file-attribute UI generally) shows for these columns/attributes
// when a user looks at a file's attributes outside Hare itself. Same text
// as GUIStrings.h's ARTIST_COLUMN..GENRE_COLUMN in the app, but this is a
// different library with its own header, so kept separate on purpose.
#define ARTIST_NAME B_TRANSLATE_CONTEXT("Artist", "LibHareStrings")
#define ALBUM_NAME B_TRANSLATE_CONTEXT("Album", "LibHareStrings")
#define TITLE_NAME B_TRANSLATE_CONTEXT("Title", "LibHareStrings")
#define YEAR_NAME B_TRANSLATE_CONTEXT("Year", "LibHareStrings")
#define COMMENT_NAME B_TRANSLATE_CONTEXT("Comment", "LibHareStrings")
#define TRACK_NAME B_TRANSLATE_CONTEXT("Track", "LibHareStrings")
#define GENRE_NAME B_TRANSLATE_CONTEXT("Genre", "LibHareStrings")

#endif
