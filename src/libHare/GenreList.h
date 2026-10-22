/*
 * Copyright 2000-2021, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __GENRE_LIST_H__
#define __GENRE_LIST_H__

// The ID3v1 genre table: Genre() turns a genre name into the ID3v1 genre
// number the encoders store. (TagKit has its own, name-only, sorted list for
// the field editor's menu; it can't give the numbers.)
class GenreList {
public:
	GenreList();
	~GenreList();

	static BString Genre(int genreindex);
	static int Genre(BString genre);
	static int NumGenres();
	static int GenreSort(const void* g1, const void* g2);
};

#endif

