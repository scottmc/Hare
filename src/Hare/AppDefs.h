/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __APP_DEFS_H__
#define __APP_DEFS_H__

#define COMPANY "HaikuArchives"
#define COMPANY_WWW "github.com/HaikuArchives/Hare"
#define APPLICATION "Hare"
// PREFS lives in GUIStrings.h now (it's user-facing text, "Preferences"),
// not a dead duplicate here - having it in both files with the same text
// used to be harmless, but once GUIStrings.h's copy became a
// B_TRANSLATE_CONTEXT() call the two macros diverged and the compiler
// started warning about the redefinition.

#define WINDOW_FILE "Hare Window"
#define SETTINGS "Hare Settings"
#define ADD_ON_DIR "add-ons"

#define SIGNATURE "application/x-vnd.haikuarchives.hare"

#define CDDA_MIME_TYPE "audio/x-cdda"

#define SYSTEM_BEEP_ENCODING_DONE "Encoding Finished"

// Build-time switch for the MusicBrainz lookup code as a whole. Whether it
// is used is the user's choice, in the preferences ("Tagging and
// Attributes").
#define ENABLE_MUSICBRAINZ_LOOKUP 1

#endif
