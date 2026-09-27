/*
 * Copyright 2000-2021, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __MP3_LAME_H__
#define __MP3_LAME_H__

#include <stdio.h>

#include "AEEncoder.h"
#include "EncoderStrings.h"

#define MP3_MIME_TYPE "audio/x-mpeg"
#define WAV_MIME_TYPE "audio/wav"
#define RIFF_WAV_MIME_TYPE "audio/x-wav"
#define RIFF_MIME_TYPE "audio/x-riff"

#define LAME "lame"
#define BITRATE_PREFIX "-b"
#define VBR_PREFIX "--vbr-new"
#define VBR_QUAL_PREFIX "-V"
#define PSYCHO_PREFIX "-q"
#define FORMAT_PREFIX "-m"
#define STEREO_CODE "s"
#define MONO_CODE "m"
#define JSTEREO_CODE "j"

class BMessage;
class BMessenger;

class MP3Lame : public AEEncoder {
public:
	MP3Lame();
	~MP3Lame();

	virtual int32 Encode(BMessage* encodeMessage);
	virtual const char* GetDefaultPattern();

protected:
	virtual int32 LoadDefaultMenu();

private:
	char lamePath[B_PATH_NAME_LENGTH+1];

	int32 GetBitrate(char* bitrate, bool* vbr);
	int32 GetFormat(char* format);
	int32 GetPsycho(char* psycho);
	int32 UpdateStatus(FILE* out, BMessenger* messenger);
	int32 WriteDetails(BMessage* encodeMessage);
	thread_id CommandIO(int* filedes, int argc, const char** argv);
};

extern char** environ;

#endif
