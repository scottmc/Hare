/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <fs_info.h>
#include <image.h>
#include <Message.h>
#include <Menu.h>
#include <MenuItem.h>
#include <File.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Volume.h>
#include <Debug.h>

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "AudioAttributes.h"

#include "FFMpegEncoder.h"

FFMpegEncoder::FFMpegEncoder() : AEEncoder(ADDON_NAME) {
	PRINT(("FFMpegEncoder::FFMpegEncoder()\n"));

	if (FindExecutable(FFMPEG, ffmpegPath) != B_OK) {
		delete menu;
		menu = 0;
		error = FSS_EXE_NOT_FOUND;
	}
}

FFMpegEncoder::~FFMpegEncoder() {
	PRINT(("FFMpegEncoder::~FFMpegEncoder()\n"));
}

int32
FFMpegEncoder::Encode(BMessage* message) {
	PRINT(("FFMpegEncoder::Encode(BMessage*)\n"));

	argument args;
	memset(&args, 0, sizeof(argument));
	char level[4];

	if (GetArgs(&args, message) != B_OK) {
		message->AddString("error", "Error getting arguments.\n");
		return B_ERROR;
	}

	if (GetCompressionLevel(level) != B_OK) {
		message->AddString("error", "Error getting compression level setting.\n");
		return B_ERROR;
	}

	//check if input file is of correct type
	BFile iFile(args.inputFile, B_READ_ONLY);
	if (iFile.InitCheck() != B_OK) {
		message->AddString("error", "Error init'ing input file.\n");
		return B_ERROR;
	}
	if (CheckAudioFileType(&iFile, true) != B_OK) {
		message->AddString("error", "Input file is not a supported WAV/AIFF file.\n");
		return FSS_INPUT_NOT_SUPPORTED;
	}

	//set up arg list -- every optional tag uses two argv slots:
	//"-metadata" followed by "key=value". Two separate slots (rather than
	//one combined string) matter here: argv entries are passed straight to
	//the new process with no shell in between, so a single argv element
	//containing an embedded space is never re-split into two arguments.
	int numTags = 0;
	if (args.artist) numTags += 2;
	if (args.album) numTags += 2;
	if (args.title) numTags += 2;
	if (args.year) numTags += 2;
	if (args.comment) numTags += 2;
	if (args.track) numTags += 2;
	if (args.genre) numTags += 2;

	const int fixedCount = 12;
	int argc = fixedCount + numTags + 1; //+1 for outputFile
	const char* argv[argc+1]; //+1 for NULL terminator

	argv[0] = ffmpegPath;
	argv[1] = OVERWRITE_PREFIX;
	argv[2] = NOSTDIN_PREFIX;
	argv[3] = HIDE_BANNER_PREFIX;
	argv[4] = INPUT_PREFIX;
	argv[5] = args.inputFile;
	argv[6] = CODEC_PREFIX;
	argv[7] = CODEC_NAME;
	argv[8] = COMPRESSION_PREFIX;
	argv[9] = level;
	argv[10] = PROGRESS_PREFIX;
	argv[11] = PROGRESS_TARGET;

	int i = fixedCount;
	if (args.artist) {
		argv[i] = strdup(METADATA_PREFIX);
		BString kv = "artist=";
		kv << args.artist;
		argv[i+1] = strdup(kv.String());
		i += 2;
	}
	if (args.album) {
		argv[i] = strdup(METADATA_PREFIX);
		BString kv = "album=";
		kv << args.album;
		argv[i+1] = strdup(kv.String());
		i += 2;
	}
	if (args.title) {
		argv[i] = strdup(METADATA_PREFIX);
		BString kv = "title=";
		kv << args.title;
		argv[i+1] = strdup(kv.String());
		i += 2;
	}
	if (args.year) {
		argv[i] = strdup(METADATA_PREFIX);
		BString kv = "date=";
		kv << args.year;
		argv[i+1] = strdup(kv.String());
		i += 2;
	}
	if (args.comment) {
		argv[i] = strdup(METADATA_PREFIX);
		BString kv = "comment=";
		kv << args.comment;
		argv[i+1] = strdup(kv.String());
		i += 2;
	}
	if (args.track) {
		argv[i] = strdup(METADATA_PREFIX);
		BString kv = "track=";
		kv << args.track;
		argv[i+1] = strdup(kv.String());
		i += 2;
	}
	if (args.genre) {
		argv[i] = strdup(METADATA_PREFIX);
		BString kv = "genre=";
		kv << args.genre;
		argv[i+1] = strdup(kv.String());
		i += 2;
	}

	argv[argc-1] = args.outputFile;
	argv[argc] = NULL;

#ifdef DEBUG
	int j = 0;
	while (argv[j] != NULL) {
		PRINT(("%s ", argv[j]));
		j++;
	}
	PRINT(("\n"));
#endif

	FILE* out;
	int filedes[2];
	thread_id ffmpeg = CommandIO(filedes, argc, argv);

	for (int k = fixedCount; k < fixedCount + numTags; k++) {
		free((void*)argv[k]);
	}

	if (ffmpeg <= B_ERROR) {
		PRINT(("ERROR: can't load ffmpeg image\n"));
		return B_ERROR;
	}
	out = fdopen(filedes[0], "r");

	resume_thread(ffmpeg);

	int32 status = UpdateStatus(out, &messenger);
	fclose(out);
	close(filedes[1]);

	if (status == FSS_CANCEL_ENCODING) {
		status = send_signal(ffmpeg, SIGTERM);
		PRINT(("status = %d\n", status));
		return FSS_CANCEL_ENCODING;
	}

	status_t err;
	wait_for_thread(ffmpeg, &err);

	WriteDetails(&args);

	return B_OK;
}

const char*
FFMpegEncoder::GetDefaultPattern() {
	PRINT(("FFMpegEncoder::GetDefaultPattern()\n"));

	BPath home;

	if (find_directory(B_USER_DIRECTORY, &home) == B_OK) {
		defaultPattern = home.Path();
		defaultPattern += "/FLAC/%a/%n/%a - %n - %k - %t.flac";
	} else {
		defaultPattern = "/boot/home/FLAC/%a/%n/%a - %n - %k - %t.flac";
	}

	return defaultPattern.String();
}

int32
FFMpegEncoder::LoadDefaultMenu() {
	PRINT(("FFMpegEncoder::LoadDefaultMenu()\n"));

	BMenu* compressionMenu;
	BMenuItem* item;

	menu = new BMenu(name.String());

	//Compression Level Menu
	compressionMenu = new BMenu(COMPRESSION_STR);
	compressionMenu->SetRadioMode(true);

	item = new BMenuItem(LEVEL0, NULL);
	compressionMenu->AddItem(item);

	item = new BMenuItem(LEVEL1, NULL);
	compressionMenu->AddItem(item);

	item = new BMenuItem(LEVEL2, NULL);
	compressionMenu->AddItem(item);

	item = new BMenuItem(LEVEL3, NULL);
	compressionMenu->AddItem(item);

	item = new BMenuItem(LEVEL4, NULL);
	compressionMenu->AddItem(item);

	item = new BMenuItem(LEVEL5, NULL);
	item->SetMarked(true);
	compressionMenu->AddItem(item);

	item = new BMenuItem(LEVEL6, NULL);
	compressionMenu->AddItem(item);

	item = new BMenuItem(LEVEL7, NULL);
	compressionMenu->AddItem(item);

	item = new BMenuItem(LEVEL8, NULL);
	compressionMenu->AddItem(item);

	menu->AddItem(compressionMenu);

	return B_OK;
}

int32
FFMpegEncoder::GetCompressionLevel(char* level) {
	PRINT(("FFMpegEncoder::GetCompressionLevel(char*)\n"));

	BMenuItem* item;
	BMenu* compressionMenu;

	item = menu->FindItem(COMPRESSION_STR);
	if (!item) {
		return B_ERROR;
	}
	compressionMenu = item->Submenu();
	if (!compressionMenu) {
		return B_ERROR;
	}

	item = compressionMenu->FindMarked();
	if (!item) {
		return B_ERROR;
	}

	const char* label = item->Label();
	if (strcmp(label, LEVEL0) == 0) {
		strcpy(level, "0");
	} else if (strcmp(label, LEVEL1) == 0) {
		strcpy(level, "1");
	} else if (strcmp(label, LEVEL2) == 0) {
		strcpy(level, "2");
	} else if (strcmp(label, LEVEL3) == 0) {
		strcpy(level, "3");
	} else if (strcmp(label, LEVEL4) == 0) {
		strcpy(level, "4");
	} else if (strcmp(label, LEVEL5) == 0) {
		strcpy(level, "5");
	} else if (strcmp(label, LEVEL6) == 0) {
		strcpy(level, "6");
	} else if (strcmp(label, LEVEL7) == 0) {
		strcpy(level, "7");
	} else if (strcmp(label, LEVEL8) == 0) {
		strcpy(level, "8");
	}

	return B_OK;
}

//ffmpeg's "-progress pipe:2" option emits well-defined, line-based
//"key=value" reports on stderr (ending each block with either
//"progress=continue" or "progress=end"), rather than a single
//continuously-overwritten \r-terminated status line. Reading it line by
//line with fgets(), and matching known key names, is far more robust than
//character-by-character scanning for substrings within a repeatedly
//rewritten line -- there is no ambiguity about where one field ends and
//another begins.
int32
FFMpegEncoder::UpdateStatus(FILE* out, BMessenger* messenger) {
	PRINT(("FFMpegEncoder::UpdateStatus(FILE*,BMessenger*)\n"));

	char line[512];
	float prev = 0.0;
	int64 totalUs = 0;
	bool haveTotal = false;

	while (1) {
		if (CheckForCancel()) {
			PRINT(("Cancel requested.\n"));
			return FSS_CANCEL_ENCODING;
		}

		if (feof(out)) {
			PRINT(("ERROR IN ENCODING STREAM - EOF ENCOUNTERED\n"));
			return B_ERROR;
		}

		if (ferror(out)) {
			PRINT(("ERROR IN ENCODING STREAM\n"));
			return B_ERROR;
		}

		if (fgets(line, sizeof(line), out) == NULL) {
			continue;
		}

		PRINT(("%s", line));

		if (!haveTotal) {
			char* durPos = strstr(line, "Duration: ");
			if (durPos) {
				int hours = 0;
				int minutes = 0;
				float seconds = 0.0f;
				if (sscanf(durPos + strlen("Duration: "), "%d:%d:%f",
						&hours, &minutes, &seconds) == 3) {
					totalUs = (int64)(((hours * 3600.0) + (minutes * 60.0)
							+ seconds) * 1000000.0);
					if (totalUs > 0) {
						haveTotal = true;
					}
				}
			}
		}

		bool haveOutTime = false;
		int64 outUs = 0;
		if (strncmp(line, "out_time_us=", 12) == 0) {
			outUs = atoll(line + 12);
			haveOutTime = true;
		} else if (strncmp(line, "out_time_ms=", 12) == 0) {
			//despite the name, ffmpeg's out_time_ms value is in
			//microseconds, matching out_time_us
			outUs = atoll(line + 12);
			haveOutTime = true;
		}

		if (haveOutTime && haveTotal && totalUs > 0) {
			float curr = ((float)outUs / (float)totalUs) * 100.0f;
			if (curr > 100.0f) {
				curr = 100.0f;
			}
			float delta = curr - prev;
			prev = curr;
			BMessage updateMessage(B_UPDATE_STATUS_BAR);
			updateMessage.AddFloat("delta", delta);
			messenger->SendMessage(&updateMessage);
		}

		if (strncmp(line, "progress=end", 12) == 0) {
			break;
		}
	}
	PRINT(("DONE ENCODING\n"));
	return B_OK;
}

int32
FFMpegEncoder::GetArgs(argument* args, BMessage* encodeMessage) {
	PRINT(("FFMpegEncoder::GetArgs(argument*,BMessage*)\n"));

	if (encodeMessage->FindString("input file", &(args->inputFile)) != B_OK) {
		return B_ERROR;
	}
	if (encodeMessage->FindString("output file", &(args->outputFile)) != B_OK) {
		return B_ERROR;
	}
	if (encodeMessage->FindMessenger("statusBarMessenger", &messenger) != B_OK) {
		return B_ERROR;
	}
	if (encodeMessage->FindString("artist", &(args->artist)) != B_OK) {
		args->artist = NULL;
	}
	if (encodeMessage->FindString("album", &(args->album)) != B_OK) {
		args->album = NULL;
	}
	if (encodeMessage->FindString("title", &(args->title)) != B_OK) {
		args->title = NULL;
	}
	if (encodeMessage->FindString("year", &(args->year)) != B_OK) {
		args->year = NULL;
	}
	if (encodeMessage->FindString("comment", &(args->comment)) != B_OK) {
		args->comment = NULL;
	}
	if (encodeMessage->FindString("track", &(args->track)) != B_OK) {
		args->track = NULL;
	}
	if (encodeMessage->FindString("genre", &(args->genre)) != B_OK) {
		args->genre = NULL;
	}

	return B_OK;
}

int32
FFMpegEncoder::WriteDetails(argument* args) {
	PRINT(("FFMpegEncoder::WriteDetails(argument*)\n"));

	dev_t device = dev_for_path(args->outputFile);
	BVolume volume(device);
	if (volume.InitCheck() != B_OK) {
		return B_ERROR;
	}

	BFile flacFile(args->outputFile, B_READ_WRITE);
	if (flacFile.InitCheck() != B_OK) {
		return B_ERROR;
	}

	if (volume.KnowsMime()) {
		BNodeInfo info(&flacFile);
		if (info.InitCheck() != B_OK) {
			return B_ERROR;
		}
		if (info.SetType(FLAC_MIME_TYPE) != B_OK) {
			return B_ERROR;
		}
	}

	if (volume.KnowsAttr()) {
		AudioAttributes attributes(&flacFile);
		attributes.SetArtist(args->artist);
		attributes.SetAlbum(args->album);
		attributes.SetTitle(args->title);
		attributes.SetYear(args->year);
		attributes.SetComment(args->comment);
		attributes.SetTrack(args->track);
		attributes.SetGenre(args->genre);
		if (attributes.Write() != B_OK) {
			return B_ERROR;
		}
	}

	return B_OK;
}

thread_id
FFMpegEncoder::CommandIO(int* filedes, int argc, const char** argv) {
	PRINT(("FFMpegEncoder::CommandIO(int*,int,const char**)\n"));

	int oldstderr;

	if (pipe(filedes) != 0) {
		PRINT(("ERROR: pipe() failed\n"));
		filedes[0] = -1;
		filedes[1] = -1;
		return B_ERROR;
	}

	oldstderr = dup(STDERR_FILENO);
	close(STDERR_FILENO);
	dup2(filedes[1], STDERR_FILENO);

	thread_id ret = load_image(argc, argv, (const char**)environ);

	dup2(oldstderr, STDERR_FILENO);
	close(oldstderr);

	if (ret < B_OK) {
		close(filedes[0]);
		close(filedes[1]);
		filedes[0] = -1;
		filedes[1] = -1;
	}

	return ret;
}

//function called by Flipside A.E. to get new AEEncoder subclass
AEEncoder*
load_encoder() {
	return new FFMpegEncoder();
}