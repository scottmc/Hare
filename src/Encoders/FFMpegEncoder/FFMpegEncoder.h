/*
 * Copyright 2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __FFMPEG_ENCODER_H__
#define __FFMPEG_ENCODER_H__

#include <stdio.h>

#include "AEEncoder.h"

#define ADDON_NAME "FFmpeg FLAC"

#define COMPRESSION_STR "Compression Level"
#define LEVEL0 "0 (Fastest)"
#define LEVEL1 "1"
#define LEVEL2 "2"
#define LEVEL3 "3"
#define LEVEL4 "4"
#define LEVEL5 "5 (Default)"
#define LEVEL6 "6"
#define LEVEL7 "7"
#define LEVEL8 "8 (Best)"

#define FLAC_MIME_TYPE "audio/x-flac"
#define WAV_MIME_TYPE "audio/wav"
#define RIFF_WAV_MIME_TYPE "audio/x-wav"
#define RIFF_MIME_TYPE "audio/x-riff"
#define AIFF_MIME_TYPE "audio/x-aiff"

#define FFMPEG "ffmpeg"
#define OVERWRITE_PREFIX "-y"
#define NOSTDIN_PREFIX "-nostdin"
#define HIDE_BANNER_PREFIX "-hide_banner"
#define INPUT_PREFIX "-i"
#define CODEC_PREFIX "-c:a"
#define CODEC_NAME "flac"
#define COMPRESSION_PREFIX "-compression_level"
#define PROGRESS_PREFIX "-progress"
#define PROGRESS_TARGET "pipe:2"
#define METADATA_PREFIX "-metadata"

extern char** environ;

struct argument {
	//required
	const char* inputFile;
	const char* outputFile;
	int32 compressionLevel;

	//optional
	const char* artist;
	const char* album;
	const char* title;
	const char* year;
	const char* comment;
	const char* track;
	const char* genre;
};

class BMessage;

class FFMpegEncoder : public AEEncoder {
public:
	FFMpegEncoder();
	~FFMpegEncoder();

	virtual int32 Encode(BMessage* encodeMessage);
	virtual const char* GetDefaultPattern();

protected:
	virtual int32 LoadDefaultMenu();

private:
	char ffmpegPath[B_PATH_NAME_LENGTH+1];
	BMessenger messenger;

	int32 GetCompressionLevel(char* level);
	int32 GetArgs(argument* args, BMessage* encodeMessage);
	int32 UpdateStatus(FILE* out, BMessenger* messenger);
	int32 WriteDetails(argument* args);
	thread_id CommandIO(int* filedes, int argc, const char** argv);
};

#endif