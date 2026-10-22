/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __COMMAND_CONSTANTS_H__
#define __COMMAND_CONSTANTS_H__

#define MENU_ITEM_SELECTED 'mnu'
#define COLUMN_SET_SHOWN '001'
#define FILE_NAME_PATTERN_CHANGED '003'
#define ENCODE_MSG '014'
#define CANCEL_MSG '015'
#define LIST_SELECTION_MSG '017'
#define SELECT_ALL_MSG '021'
#define DESELECT_ALL_MSG '022'
#define REMOVE_MSG '023'
#define PREFS_MSG '024'
#define ENCODER_CHANGED '026'
#define VIEW_SHORTCUT '027'
#define SAVE_PREFS '028'
#define REVERT_PREFS '029'
#define ENCODER_PATTERN_PREFS '030'
#define SAVE_LAYOUT_MSG '031'
#define COVER_ART_FOUND '032'
#define METADATA_FOUND '033'
#define MUSICBRAINZ_LOOKUP_STARTED '034'
#define MUSICBRAINZ_LOOKUP_FINISHED '035'
#define CD_MOUNT_STATE_CHANGED '036'
#define LOAD_CD_BUTTON_PRESSED '037'
#define COVER_ART_CONTEXT '038'
#define COVER_ART_CHOOSE '039'
// compactView asks for a field to be edited / the field editor's result.
#define FIELD_EDIT_REQUESTED '040'
#define FIELD_EDITED '041'
// A checkbox of the preferences window's Tagging and Attributes box
// ("option" is the pref_option) / its cover image format menu ("mime").
#define PREF_OPTION_CHANGED '042'
#define PREF_COVER_FORMAT '043'

// Cap on how many MusicBrainz cover-art candidates get fetched and
// offered to the user to choose between - see TagKit's
// CoverArtPickerWindow and MusicBrainzLookup::FetchThread()'s own comments.
#define MAX_COVER_ART_CANDIDATES 10

#define NUM_OF_COLUMNS 10
#define COMPLETE_COLUMN_INDEX 0
#define FILE_COLUMN_INDEX 1
#define SAVE_AS_COLUMN_INDEX 2
#define ARTIST_COLUMN_INDEX 3
#define ALBUM_COLUMN_INDEX 4
#define TITLE_COLUMN_INDEX 5
#define TRACK_COLUMN_INDEX 6
#define YEAR_COLUMN_INDEX 7
#define COMMENT_COLUMN_INDEX 8
#define GENRE_COLUMN_INDEX 9

#endif
