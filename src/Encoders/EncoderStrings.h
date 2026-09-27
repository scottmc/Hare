/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __ENCODER_STRINGS_H__
#define __ENCODER_STRINGS_H__

#include <Catalog.h>

// Shared by the encoder add-ons that have an include path to this file
// (see each encoder's Makefile LOCAL_INCLUDE_PATHS). Each encoder is its
// own separate build target/binary, so identical English text between
// encoders (e.g. "Bitrate") still gets its own define here, prefixed per
// encoder - the same text today doesn't mean it has to stay identical if
// one encoder's wording needs to change later.

// ---- FFMpegEncoder ----
#define FFMPEG_ADDON_NAME_TXT B_TRANSLATE_CONTEXT("FFmpeg FLAC", "FFMpegEncoder")

#define FFMPEG_COMPRESSION_STR B_TRANSLATE_CONTEXT("Compression Level", "FFMpegEncoder")
#define FFMPEG_LEVEL0_TXT B_TRANSLATE_CONTEXT("0 (Fastest)", "FFMpegEncoder")
#define FFMPEG_LEVEL1_TXT B_TRANSLATE_CONTEXT("1", "FFMpegEncoder")
#define FFMPEG_LEVEL2_TXT B_TRANSLATE_CONTEXT("2", "FFMpegEncoder")
#define FFMPEG_LEVEL3_TXT B_TRANSLATE_CONTEXT("3", "FFMpegEncoder")
#define FFMPEG_LEVEL4_TXT B_TRANSLATE_CONTEXT("4", "FFMpegEncoder")
#define FFMPEG_LEVEL5_TXT B_TRANSLATE_CONTEXT("5 (Default)", "FFMpegEncoder")
#define FFMPEG_LEVEL6_TXT B_TRANSLATE_CONTEXT("6", "FFMpegEncoder")
#define FFMPEG_LEVEL7_TXT B_TRANSLATE_CONTEXT("7", "FFMpegEncoder")
#define FFMPEG_LEVEL8_TXT B_TRANSLATE_CONTEXT("8 (Best)", "FFMpegEncoder")

#define FFMPEG_ERROR_ARGS_TXT B_TRANSLATE_CONTEXT("Error getting arguments.\n", "FFMpegEncoder")
#define FFMPEG_ERROR_COMPRESSION_SETTING_TXT B_TRANSLATE_CONTEXT("Error getting compression level setting.\n", "FFMpegEncoder")
#define FFMPEG_ERROR_INIT_INPUT_TXT B_TRANSLATE_CONTEXT("Error init'ing input file.\n", "FFMpegEncoder")
#define FFMPEG_ERROR_UNSUPPORTED_INPUT_TXT B_TRANSLATE_CONTEXT("Input file is not a supported WAV/AIFF file.\n", "FFMpegEncoder")

// ---- MP3GoGo (GoGoEncoder) ----
#define GOGO_ADDON_NAME_TXT B_TRANSLATE_CONTEXT("MP3 GoGo", "GoGoEncoder")

#define GOGO_BITRATE_STR B_TRANSLATE_CONTEXT("Bitrate", "GoGoEncoder")
#define GOGO_BR_32_TXT B_TRANSLATE_CONTEXT(" 32 Kbps                             ", "GoGoEncoder")
#define GOGO_BR_48_TXT B_TRANSLATE_CONTEXT(" 48 Kbps                             ", "GoGoEncoder")
#define GOGO_BR_64_TXT B_TRANSLATE_CONTEXT(" 64 Kbps                             ", "GoGoEncoder")
#define GOGO_BR_96_TXT B_TRANSLATE_CONTEXT(" 96 Kbps                             ", "GoGoEncoder")
#define GOGO_BR_128_TXT B_TRANSLATE_CONTEXT("128 Kbps                             ", "GoGoEncoder")
#define GOGO_BR_160_TXT B_TRANSLATE_CONTEXT("160 Kbps                             ", "GoGoEncoder")
#define GOGO_BR_192_TXT B_TRANSLATE_CONTEXT("192 Kbps                             ", "GoGoEncoder")
#define GOGO_BR_256_TXT B_TRANSLATE_CONTEXT("256 Kbps                             ", "GoGoEncoder")
#define GOGO_BR_320_TXT B_TRANSLATE_CONTEXT("320 Kbps                             ", "GoGoEncoder")
#define GOGO_VBR_0_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 0 (High Quality)    ", "GoGoEncoder")
#define GOGO_VBR_1_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 1                   ", "GoGoEncoder")
#define GOGO_VBR_2_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 2                   ", "GoGoEncoder")
#define GOGO_VBR_3_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 3                   ", "GoGoEncoder")
#define GOGO_VBR_4_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 4                   ", "GoGoEncoder")
#define GOGO_VBR_5_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 5                   ", "GoGoEncoder")
#define GOGO_VBR_6_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 6                   ", "GoGoEncoder")
#define GOGO_VBR_7_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 7                   ", "GoGoEncoder")
#define GOGO_VBR_8_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 8                   ", "GoGoEncoder")
#define GOGO_VBR_9_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 9 (High Compression)", "GoGoEncoder")

#define GOGO_OUTPUT_FORMAT_STR B_TRANSLATE_CONTEXT("Output Format", "GoGoEncoder")
#define GOGO_STEREO_TXT B_TRANSLATE_CONTEXT("Stereo  ", "GoGoEncoder")
#define GOGO_MONO_TXT B_TRANSLATE_CONTEXT("Mono    ", "GoGoEncoder")
#define GOGO_JSTEREO_TXT B_TRANSLATE_CONTEXT("J-Stereo", "GoGoEncoder")

#define GOGO_PSYCHO_ACOUSTICS_STR B_TRANSLATE_CONTEXT("Psycho Acoustics", "GoGoEncoder")

#define GOGO_ERROR_INPUT_PATH_TXT B_TRANSLATE_CONTEXT("Error getting input path.\n", "GoGoEncoder")
#define GOGO_ERROR_OUTPUT_PATH_TXT B_TRANSLATE_CONTEXT("Error getting output path.\n", "GoGoEncoder")
#define GOGO_ERROR_STATUSBAR_TXT B_TRANSLATE_CONTEXT("Error getting status bar messenger.\n", "GoGoEncoder")
#define GOGO_ERROR_BITRATE_SETTING_TXT B_TRANSLATE_CONTEXT("Error getting bitrate setting.\n", "GoGoEncoder")
#define GOGO_ERROR_FORMAT_SETTING_TXT B_TRANSLATE_CONTEXT("Error getting format setting.\n", "GoGoEncoder")
#define GOGO_ERROR_PSYCHOACOUSTIC_SETTING_TXT B_TRANSLATE_CONTEXT("Error getting psychoacoustic setting.\n", "GoGoEncoder")
#define GOGO_ERROR_INIT_INPUT_TXT B_TRANSLATE_CONTEXT("Error init'ing input file.\n", "GoGoEncoder")
#define GOGO_ERROR_UNSUPPORTED_INPUT_TXT B_TRANSLATE_CONTEXT("Input file is not a supported WAV file.\n", "GoGoEncoder")
#define GOGO_ERROR_RUNNING_TXT B_TRANSLATE_CONTEXT("Error running gogo.\n", "GoGoEncoder")

// ---- MP3Lame ----
#define LAME_ADDON_NAME_TXT B_TRANSLATE_CONTEXT("MP3 Lame", "MP3Lame")

#define LAME_BITRATE_STR B_TRANSLATE_CONTEXT("Bitrate", "MP3Lame")
#define LAME_BR_32_TXT B_TRANSLATE_CONTEXT(" 32 Kbps                             ", "MP3Lame")
#define LAME_BR_48_TXT B_TRANSLATE_CONTEXT(" 48 Kbps                             ", "MP3Lame")
#define LAME_BR_64_TXT B_TRANSLATE_CONTEXT(" 64 Kbps                             ", "MP3Lame")
#define LAME_BR_96_TXT B_TRANSLATE_CONTEXT(" 96 Kbps                             ", "MP3Lame")
#define LAME_BR_128_TXT B_TRANSLATE_CONTEXT("128 Kbps                             ", "MP3Lame")
#define LAME_BR_160_TXT B_TRANSLATE_CONTEXT("160 Kbps                             ", "MP3Lame")
#define LAME_BR_192_TXT B_TRANSLATE_CONTEXT("192 Kbps                             ", "MP3Lame")
#define LAME_BR_256_TXT B_TRANSLATE_CONTEXT("256 Kbps                             ", "MP3Lame")
#define LAME_BR_320_TXT B_TRANSLATE_CONTEXT("320 Kbps                             ", "MP3Lame")
#define LAME_VBR_0_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 0 (High Quality)    ", "MP3Lame")
#define LAME_VBR_1_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 1                   ", "MP3Lame")
#define LAME_VBR_2_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 2                   ", "MP3Lame")
#define LAME_VBR_3_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 3                   ", "MP3Lame")
#define LAME_VBR_4_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 4 (Default)         ", "MP3Lame")
#define LAME_VBR_5_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 5                   ", "MP3Lame")
#define LAME_VBR_6_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 6                   ", "MP3Lame")
#define LAME_VBR_7_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 7                   ", "MP3Lame")
#define LAME_VBR_8_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 8                   ", "MP3Lame")
#define LAME_VBR_9_TXT B_TRANSLATE_CONTEXT("Variable Bitrate 9 (High Compression)", "MP3Lame")

#define LAME_OUTPUT_FORMAT_STR B_TRANSLATE_CONTEXT("Output Format", "MP3Lame")
#define LAME_STEREO_TXT B_TRANSLATE_CONTEXT("Stereo  ", "MP3Lame")
#define LAME_MONO_TXT B_TRANSLATE_CONTEXT("Mono    ", "MP3Lame")
#define LAME_JSTEREO_TXT B_TRANSLATE_CONTEXT("J-Stereo", "MP3Lame")

#define LAME_PSYCHO_ACOUSTICS_STR B_TRANSLATE_CONTEXT("Psycho Acoustics", "MP3Lame")
#define LAME_PSY_0_TXT B_TRANSLATE_CONTEXT("Psycho Acoustics 0 (High Quality, Slow)", "MP3Lame")
#define LAME_PSY_1_TXT B_TRANSLATE_CONTEXT("Psycho Acoustics 1                     ", "MP3Lame")
#define LAME_PSY_2_TXT B_TRANSLATE_CONTEXT("Psycho Acoustics 2 (Recommended)       ", "MP3Lame")
#define LAME_PSY_3_TXT B_TRANSLATE_CONTEXT("Psycho Acoustics 3                     ", "MP3Lame")
#define LAME_PSY_4_TXT B_TRANSLATE_CONTEXT("Psycho Acoustics 4                     ", "MP3Lame")
#define LAME_PSY_5_TXT B_TRANSLATE_CONTEXT("Psycho Acoustics 5 (Default)           ", "MP3Lame")
#define LAME_PSY_6_TXT B_TRANSLATE_CONTEXT("Psycho Acoustics 6                     ", "MP3Lame")
#define LAME_PSY_7_TXT B_TRANSLATE_CONTEXT("Psycho Acoustics 7 (OK Quality, Fast)  ", "MP3Lame")
#define LAME_PSY_8_TXT B_TRANSLATE_CONTEXT("Psycho Acoustics 8                     ", "MP3Lame")
#define LAME_PSY_9_TXT B_TRANSLATE_CONTEXT("Psycho Acoustics 9 (Poor Quality, Fast)", "MP3Lame")

#define LAME_ERROR_INPUT_PATH_TXT B_TRANSLATE_CONTEXT("Error getting input path.\n", "MP3Lame")
#define LAME_ERROR_OUTPUT_PATH_TXT B_TRANSLATE_CONTEXT("Error getting output path.\n", "MP3Lame")
#define LAME_ERROR_STATUSBAR_TXT B_TRANSLATE_CONTEXT("Error getting status bar messenger.\n", "MP3Lame")
#define LAME_ERROR_BITRATE_SETTING_TXT B_TRANSLATE_CONTEXT("Error getting bitrate setting.\n", "MP3Lame")
#define LAME_ERROR_FORMAT_SETTING_TXT B_TRANSLATE_CONTEXT("Error getting format setting.\n", "MP3Lame")
#define LAME_ERROR_PSYCHOACOUSTIC_SETTING_TXT B_TRANSLATE_CONTEXT("Error getting psychoacoustic setting.\n", "MP3Lame")
#define LAME_ERROR_INIT_INPUT_TXT B_TRANSLATE_CONTEXT("Error init'ing input file.\n", "MP3Lame")
#define LAME_ERROR_UNSUPPORTED_INPUT_TXT B_TRANSLATE_CONTEXT("Input file is not a supported WAV file.\n", "MP3Lame")
#define LAME_ERROR_RUNNING_TXT B_TRANSLATE_CONTEXT("Error running lame.\n", "MP3Lame")

// ---- OGGEncoder ----
#define OGG_ADDON_NAME_TXT B_TRANSLATE_CONTEXT("OGG Encoder", "OggEncoder")

#define OGG_BITRATE_STR B_TRANSLATE_CONTEXT("Bitrate", "OggEncoder")
#define OGG_BR_112_TXT B_TRANSLATE_CONTEXT("112 Kbps", "OggEncoder")
#define OGG_BR_128_TXT B_TRANSLATE_CONTEXT("128 Kbps", "OggEncoder")
#define OGG_BR_160_TXT B_TRANSLATE_CONTEXT("160 Kbps", "OggEncoder")
#define OGG_BR_192_TXT B_TRANSLATE_CONTEXT("192 Kbps", "OggEncoder")
#define OGG_BR_256_TXT B_TRANSLATE_CONTEXT("256 Kbps", "OggEncoder")
#define OGG_BR_320_TXT B_TRANSLATE_CONTEXT("320 Kbps", "OggEncoder")

#define OGG_ERROR_ARGS_TXT B_TRANSLATE_CONTEXT("Error getting arguments.\n", "OggEncoder")
#define OGG_ERROR_BITRATE_SETTING_TXT B_TRANSLATE_CONTEXT("Error getting bitrate setting.\n", "OggEncoder")
#define OGG_ERROR_INIT_INPUT_TXT B_TRANSLATE_CONTEXT("Error init'ing input file.\n", "OggEncoder")
#define OGG_ERROR_UNSUPPORTED_INPUT_TXT B_TRANSLATE_CONTEXT("Input file is not a supported WAV/AIFF file.\n", "OggEncoder")

#endif
