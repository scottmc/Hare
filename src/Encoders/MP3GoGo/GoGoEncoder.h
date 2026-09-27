/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __GOGO_ENCODER_H__
#define __GOGO_ENCODER_H__

#include <stdio.h>

#include "AEEncoder.h"
#include "EncoderStrings.h"

#define MP3_MIME_TYPE "audio/x-mpeg"
#define WAV_MIME_TYPE "audio/wav"
#define RIFF_WAV_MIME_TYPE "audio/x-wav"
#define RIFF_MIME_TYPE "audio/x-riff"

#define GOGO "gogo"
#define BITRATE_PREFIX "-b"
#define VBR_PREFIX "-v"
#define FORMAT_PREFIX "-m"
#define STEREO_CODE "s"
#define MONO_CODE "m"
#define JSTEREO_CODE "j"

class BMessage;
class BMessenger;

class GoGoEncoder : public AEEncoder {
public:
	GoGoEncoder();
	~GoGoEncoder();

	virtual int32 Encode(BMessage* encodeMessage);
	virtual const char* GetDefaultPattern();

protected:
	virtual int32 LoadDefaultMenu();

private:
	char gogoPath[B_PATH_NAME_LENGTH+1];

	int32 GetBitrate(char* bitrate, bool* vbr);
	int32 GetFormat(char* format);
	int32 GetPsycho(bool* psycho);
	int32 UpdateStatus(FILE* out, BMessenger* messenger);
	int32 WriteDetails(BMessage* encodeMessage);
	thread_id CommandIO(int* filedes, int argc, const char** argv);
};

extern char** environ;

#endif
