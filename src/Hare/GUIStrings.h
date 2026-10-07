/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __GUI_STRINGS_H__
#define __GUI_STRINGS_H__

#include "AppDefs.h"
#include <Catalog.h>

#define ENCODE_BTN B_TRANSLATE_CONTEXT("Encode", "GUIStrings")
#define CANCEL_BTN B_TRANSLATE_CONTEXT("Cancel", "GUIStrings")
#define APPLY_BTN B_TRANSLATE_CONTEXT("Apply", "GUIStrings")
#define REVERT_BTN B_TRANSLATE_CONTEXT("Revert", "GUIStrings")
#define ENCODER_PATTERN_BTN B_TRANSLATE_CONTEXT("Use Encoder Pattern", "GUIStrings")

#define STATUS_LABEL B_TRANSLATE_CONTEXT("Encoding Song: ", "GUIStrings")
#define RIPPING_LABEL B_TRANSLATE_CONTEXT("Ripping Song: ", "GUIStrings")
#define STATUS_TRAILING_LABEL B_TRANSLATE_CONTEXT("Songs Remaining: ", "GUIStrings")

#define MUSICBRAINZ_LOOKUP_LABEL_TXT B_TRANSLATE_CONTEXT("Looking up disc info...", "GUIStrings")

#define OK B_TRANSLATE_CONTEXT("Ok", "GUIStrings")
#define YES B_TRANSLATE_CONTEXT("Yes", "GUIStrings")
#define NO B_TRANSLATE_CONTEXT("No", "GUIStrings")

#define FILE_MENU B_TRANSLATE_CONTEXT("File", "FileMenu")
#define LOAD_CD_SUBMENU B_TRANSLATE_CONTEXT("Load CD", "LoadCdMenu")
#define NO_CD_MOUNTED B_TRANSLATE_CONTEXT("No CD Mounted", "GUIStrings")
#define ABOUT_MENU_ITEM B_TRANSLATE_CONTEXT("About...", "GUIStrings")
#define QUIT_MENU_ITEM B_TRANSLATE_CONTEXT("Quit", "GUIStrings")

#define EDIT_MENU B_TRANSLATE_CONTEXT("Edit", "GUIStrings")
#define SELECT_ALL B_TRANSLATE_CONTEXT("Select All", "GUIStrings")
#define DESELECT_ALL B_TRANSLATE_CONTEXT("Deselect All", "GUIStrings")
#define REMOVE B_TRANSLATE_CONTEXT("Remove", "GUIStrings")
#define SAVE_LAYOUT B_TRANSLATE_CONTEXT("Save Layout", "GUIStrings")
#define PREFS B_TRANSLATE_CONTEXT("Preferences", "GUIStrings")

#define COLUMN_MENU B_TRANSLATE_CONTEXT("Columns", "GUIStrings")
#define FILE_COLUMN B_TRANSLATE_CONTEXT("File", "FileColumn")
#define SAVE_AS_COLUMN B_TRANSLATE_CONTEXT("Save As", "GUIStrings")
#define ARTIST_COLUMN B_TRANSLATE_CONTEXT("Artist", "GUIStrings")
#define ALBUM_COLUMN B_TRANSLATE_CONTEXT("Album", "GUIStrings")
#define TITLE_COLUMN B_TRANSLATE_CONTEXT("Title", "GUIStrings")
#define YEAR_COLUMN B_TRANSLATE_CONTEXT("Year", "GUIStrings")
#define COMMENT_COLUMN B_TRANSLATE_CONTEXT("Comment", "GUIStrings")
#define TRACK_COLUMN B_TRANSLATE_CONTEXT("Track", "GUIStrings")
#define GENRE_COLUMN B_TRANSLATE_CONTEXT("Genre", "GUIStrings")

#define ARTIST_LABEL B_TRANSLATE_CONTEXT("Artist = %a", "GUIStrings")
#define ALBUM_LABEL B_TRANSLATE_CONTEXT("Album = %n", "GUIStrings")
#define TITLE_LABEL B_TRANSLATE_CONTEXT("Title = %t", "GUIStrings")
#define YEAR_LABEL B_TRANSLATE_CONTEXT("Year = %y", "GUIStrings")
#define COMMENT_LABEL B_TRANSLATE_CONTEXT("Comment = %c", "GUIStrings")
#define TRACK_LABEL B_TRANSLATE_CONTEXT("Track = %k", "GUIStrings")
#define GENRE_LABEL B_TRANSLATE_CONTEXT("Genre = %g", "GUIStrings")

#define ENCODER_MENU B_TRANSLATE_CONTEXT("Encoder", "GUIStrings")
#define NO_AVAILABLE_ENCODERS B_TRANSLATE_CONTEXT("No Available Encoders", "GUIStrings")

#define FILE_NAME_PATTERN_BOX_LABEL B_TRANSLATE_CONTEXT("File Path and Name Pattern", "GUIStrings")
#define FILE_NAME_PATTERN_BOX_LABEL_FOR B_TRANSLATE_CONTEXT("File Path and Name Pattern for ", "GUIString")
#define FILE_NAME_PATTERN_LABEL B_TRANSLATE_CONTEXT("Save As:", "GUIStrings")

#define EDITOR_LABEL B_TRANSLATE_CONTEXT("Details Editor", "GUIStrings")

#define REMOVED_TXT B_TRANSLATE_CONTEXT("One or more items in the list have been deleted.\nEncoding has been canceled.", "GUIStrings")

#define PICK_COVER_ART_LABEL B_TRANSLATE_CONTEXT("Pick cover art before encoding", "GUIStrings")

#define LOW_DISK_SPACE_PREFIX_TXT B_TRANSLATE_CONTEXT("Low disk space: only about ", "GUIStrings")
#define LOW_DISK_SPACE_MIDDLE_TXT B_TRANSLATE_CONTEXT(" MB free on the destination volume, but this encode run may need roughly ", "GUIStrings")
#define LOW_DISK_SPACE_SUFFIX_TXT B_TRANSLATE_CONTEXT(" MB.\n\nYou can close this and continue if you like - this is just a warning.", "GUIStrings")

#define ENCODER_EXE_NOT_FOUND_INIT_TXT B_TRANSLATE_CONTEXT("The addon could not find the required executable.", "EncoderExeNotFoundInit")
#define ENCODER_EXE_NOT_FOUND_INIT_DOC_TXT B_TRANSLATE_CONTEXT("Please check the addon's documentation.\n", "EncoderExeNotFoundInitDoc")
#define ENCODER_EXE_NOT_FOUND_RUN_TXT B_TRANSLATE_CONTEXT("The addon could not find the required executable.", "EncoderExeNotFoundRun")
#define ENCODER_EXE_NOT_FOUND_RUN_DOC_TXT B_TRANSLATE_CONTEXT("Please check the addon's documentation.\n", "EncoderExeNotFoundRunDoc")

#define ERROR_OPENING_FILE_ENTRY_TXT B_TRANSLATE_CONTEXT("Error opening file: ", "ErrorOpeningFileEntry")
#define ERROR_OPENING_FILE_PATH_TXT B_TRANSLATE_CONTEXT("Error opening file: ", "ErrorOpeningFilePath")
#define ERROR_CREATING_OUTPUT_PATH_TXT B_TRANSLATE_CONTEXT("Error creating path for: ", "ErrorCreatingOutputPath")
#define ERROR_CREATING_OUTPUT_PARENT_TXT B_TRANSLATE_CONTEXT("Error creating path for: ", "ErrorCreatingOutputParent")
#define ERROR_CREATING_OUTPUT_DIR_TXT B_TRANSLATE_CONTEXT("Error creating path for: ", "ErrorCreatingOutputDir")

#define CD_TIMEOUT_PREFIX_TXT B_TRANSLATE_CONTEXT("The CD drive stopped responding while ripping ", "GUIStrings")
#define CD_TIMEOUT_MIDDLE_TXT B_TRANSLATE_CONTEXT(" (no data for over ", "GUIStrings")
#define CD_TIMEOUT_SUFFIX_TXT B_TRANSLATE_CONTEXT(" seconds). This usually means the disc was ejected or the drive was disconnected.\n\nEncoding has been stopped.", "GUIStrings")

#define CD_MEDIA_REMOVED_PREFIX_TXT B_TRANSLATE_CONTEXT("The disc was removed while ripping ", "GUIStrings")
#define CD_MEDIA_REMOVED_SUFFIX_TXT B_TRANSLATE_CONTEXT(".\n\nEncoding has been stopped.", "GUIStrings")

#define INPUT_NOT_SUPPORTED_PREFIX_TXT B_TRANSLATE_CONTEXT("Input File: ", "GUIStrings")
#define INPUT_NOT_SUPPORTED_SUFFIX_TXT B_TRANSLATE_CONTEXT(" cannot be encoded with this encoder. Continue?", "GUIStrings")

#define ERROR_ENCODING_FILE_TXT B_TRANSLATE_CONTEXT("Error encoding file: ", "GUIStrings")
#define ERROR_ENCODING_CANNOT_CONTINUE_TXT B_TRANSLATE_CONTEXT("Cannot continue.", "GUIStrings")

#define OLD_HAIKU_PREFIX_TXT B_TRANSLATE_CONTEXT("This copy of Haiku (", "GUIStrings")
#define OLD_HAIKU_HREV_TXT B_TRANSLATE_CONTEXT(") predates the SCSI CD driver fixes in hrev", "GUIStrings")
#define OLD_HAIKU_DISABLED_IN_TXT B_TRANSLATE_CONTEXT(". Loading a CD on an older Haiku can crash the whole system, so CD support has been disabled in ", "GUIStrings")
#define OLD_HAIKU_UNTIL_UPDATE_TXT B_TRANSLATE_CONTEXT(" until you update.\n\nYou can still encode existing audio files.", "GUIStrings")
#define CD_DISABLED_MENU_ITEM_TXT B_TRANSLATE_CONTEXT("CD support disabled (Haiku too old)", "GUIStrings")

#define VERSION_VARIETY_DEVELOPMENT_TXT B_TRANSLATE_CONTEXT("development", "GUIStrings")
#define VERSION_VARIETY_ALPHA_TXT B_TRANSLATE_CONTEXT("alpha", "GUIStrings")
#define VERSION_VARIETY_BETA_TXT B_TRANSLATE_CONTEXT("beta", "GUIStrings")
#define VERSION_VARIETY_GAMMA_TXT B_TRANSLATE_CONTEXT("gamma", "GUIStrings")
#define VERSION_VARIETY_GOLDEN_MASTER_TXT B_TRANSLATE_CONTEXT("golden master", "GUIStrings")

#define LOAD_CD_BUTTON_TXT B_TRANSLATE_CONTEXT("Load CD", "LoadCdButton")

#define OTHER_GENRE_TXT B_TRANSLATE_CONTEXT("Other", "GUIStrings")

#define ICON_COLUMN B_TRANSLATE_CONTEXT("Icon", "GUIStrings")

#endif
