/*
 * Copyright 2000-2026, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

#include "AudioAttributes.h"
#include "ID3Tags.h"

#include "GoGoEncoder.h"

GoGoEncoder::GoGoEncoder() : AEEncoder(GOGO_ADDON_NAME_TXT) {
	PRINT(("GoGoEncoder::GoGoEncoder()\n"));

	if (FindExecutable(GOGO, gogoPath) != B_OK) {
		delete menu;
		menu = 0;
		error = FSS_EXE_NOT_FOUND;
	}

	if (menu) {
		BMenuItem* item = menu->FindItem(GOGO_BR_32_TXT);
		if (!item) {
			delete menu;
			menu = 0;
		}
	}
}

GoGoEncoder::~GoGoEncoder() {
	PRINT(("GoGoEncoder::~GoGoEncoder()\n"));
}

int32
GoGoEncoder::Encode(BMessage* message) {
	PRINT(("GoGoEncoder::Encode(BMessage*)\n"));

	const char* inputFile;
	const char* outputFile;
	char bitrate[4];
	bool vbr;
	char format[2];
	bool psycho;
	BMessenger messenger;

	//set required fields; quit if not found or error
	if (message->FindString("input file", &inputFile) != B_OK) {
		message->AddString("error", GOGO_ERROR_INPUT_PATH_TXT);
		return B_ERROR;
	}
	if (message->FindString("output file", &outputFile) != B_OK) {
		message->AddString("error", GOGO_ERROR_OUTPUT_PATH_TXT);
		return B_ERROR;
	}
	if (message->FindMessenger("statusBarMessenger", &messenger) != B_OK) {
		message->AddString("error", GOGO_ERROR_STATUSBAR_TXT);
		return B_ERROR;
	}
	if (GetBitrate(bitrate, &vbr) != B_OK) {
		message->AddString("error", GOGO_ERROR_BITRATE_SETTING_TXT);
		return B_ERROR;
	}
	if (GetFormat(format) != B_OK) {
		message->AddString("error", GOGO_ERROR_FORMAT_SETTING_TXT);
		return B_ERROR;
	}
	if (GetPsycho(&psycho) != B_OK) {
		message->AddString("error", GOGO_ERROR_PSYCHOACOUSTIC_SETTING_TXT);
		return B_ERROR;
	}

	//check if input file is of correct type
	BFile iFile(inputFile, B_READ_ONLY);
	if (iFile.InitCheck() != B_OK) {
		message->AddString("error", GOGO_ERROR_INIT_INPUT_TXT);
		return B_ERROR;
	}
	if (CheckAudioFileType(&iFile) != B_OK) {
		message->AddString("error", GOGO_ERROR_UNSUPPORTED_INPUT_TXT);
		return FSS_INPUT_NOT_SUPPORTED;
	}

	//set up arg list
	int argc = 7;
	if (!psycho) {
		argc = 8;
	}
	const char* argv[argc+1];

	argv[0] = gogoPath;
	argv[1] = inputFile;
	argv[2] = outputFile;
	if (vbr) {
		argv[3] = VBR_PREFIX;
	} else {
		argv[3] = BITRATE_PREFIX;
	}
	argv[4] = bitrate;
	argv[5] = FORMAT_PREFIX;
	argv[6] = format;

	if (!psycho) {
		argv[7] = "-nopsy";
		argv[8] = NULL;
	} else {
		argv[7] = NULL;
	}

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
	thread_id gogo = CommandIO(filedes, argc, argv);
	if (gogo <= B_ERROR) {
		message->AddString("error", GOGO_ERROR_RUNNING_TXT);
		return B_ERROR;
	}
	out = fdopen(filedes[0], "r");

	resume_thread(gogo);

	int32 status = UpdateStatus(out, &messenger);

	close(filedes[1]);
	close(filedes[0]);

	if (status == FSS_CANCEL_ENCODING) {
		status = send_signal(gogo, SIGTERM);
		PRINT(("status = %d\n", status));
		return FSS_CANCEL_ENCODING;
	}

	status_t err;
	wait_for_thread(gogo, &err);

	WriteDetails(message);

	if (CheckForCancel()) {
		PRINT(("Cancel Requested.\n"));
		return FSS_CANCEL_ENCODING;
	}

	return B_OK;
}

const char*
GoGoEncoder::GetDefaultPattern() {
	PRINT(("GoGoEncoder::LoadDefaultPattern()\n"));

	BPath home;
	BString pattern;

	if (find_directory(B_USER_DIRECTORY, &home) == B_OK) {
		pattern += home.Path();
		pattern += "/MP3/%a/%n/%a - %n - %k - %t.mp3";
	} else {
		pattern = "/boot/home/MP3/%a/%n/%a - %n - %k - %t.mp3";
	}
	
	return pattern.String();
}

int32
GoGoEncoder::LoadDefaultMenu() {
	PRINT(("GoGoEncoder::LoadDefaultMenu()\n"));

	BMenu* bitrateMenu;
	BMenuItem* item;

	menu = new BMenu(name.String());

	//Bitrate Menu
	bitrateMenu = new BMenu(GOGO_BITRATE_STR);
	bitrateMenu->SetRadioMode(true);
	item = new BMenuItem(GOGO_BR_32_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_BR_48_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_BR_64_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_BR_96_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_BR_128_TXT, NULL);
	item->SetMarked(true);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_BR_160_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_BR_192_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_BR_256_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_BR_320_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_VBR_0_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_VBR_1_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_VBR_2_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_VBR_3_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_VBR_4_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_VBR_5_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_VBR_6_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_VBR_7_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_VBR_8_TXT, NULL);
	bitrateMenu->AddItem(item);
	item = new BMenuItem(GOGO_VBR_9_TXT, NULL);
	bitrateMenu->AddItem(item);
	menu->AddItem(bitrateMenu);

	//Output Format Menu
	BMenu* outputFormatMenu = new BMenu(GOGO_OUTPUT_FORMAT_STR);
	outputFormatMenu->SetRadioMode(true);
	item = new BMenuItem(GOGO_STEREO_TXT, NULL);
	item->SetMarked(true);
	outputFormatMenu->AddItem(item);
	item = new BMenuItem(GOGO_MONO_TXT, NULL);
	outputFormatMenu->AddItem(item);
	item = new BMenuItem(GOGO_JSTEREO_TXT, NULL);
	outputFormatMenu->AddItem(item);
	menu->AddItem(outputFormatMenu);

	//Misc Items
	item = new BMenuItem(GOGO_PSYCHO_ACOUSTICS_STR,
						 new BMessage(FSS_MENU_ITEM_SELECTED));
	item->SetMarked(true);
	menu->AddItem(item);
	
	return B_OK;
}

int32
GoGoEncoder::GetBitrate(char* bitrate, bool* vbr) {
	PRINT(("GoGoEncoder::GetBitrate(char*,bool*)\n"));

	BMenuItem* item;
	BMenu* bitrateMenu;

	item = menu->FindItem(GOGO_BITRATE_STR);
	if (!item) {
		return B_ERROR;
	}
	bitrateMenu = item->Submenu();
	if (!bitrateMenu) {
		return B_ERROR;
	}

	item = bitrateMenu->FindMarked();
	if (!item) {
		return B_ERROR;
	}

	const char* label = item->Label();
	if (strcmp(label, GOGO_BR_32_TXT) == 0) {
		*vbr = false;
		strcpy(bitrate, "32");
	}
	if (strcmp(label, GOGO_BR_48_TXT) == 0) {
		*vbr = false;
		strcpy(bitrate, "48");
	}
	if (strcmp(label, GOGO_BR_64_TXT) == 0) {
		*vbr = false;
		strcpy(bitrate, "64");
	} else if (strcmp(label, GOGO_BR_96_TXT) == 0) {
		*vbr = false;
		strcpy(bitrate, "96");
	} else if (strcmp(label, GOGO_BR_128_TXT) == 0) {
		*vbr = false;
		strcpy(bitrate, "128");
	} else if (strcmp(label, GOGO_BR_160_TXT) == 0) {
		*vbr = false;
		strcpy(bitrate, "160");
	} else if (strcmp(label, GOGO_BR_192_TXT) == 0) {
		*vbr = false;
		strcpy(bitrate, "192");
	} else if (strcmp(label, GOGO_BR_256_TXT) == 0) {
		*vbr = false;
		strcpy(bitrate, "256");
	} else if (strcmp(label, GOGO_BR_320_TXT) == 0) {
		*vbr = false;
		strcpy(bitrate, "320");
	} else if (strcmp(label, GOGO_VBR_0_TXT) == 0) {
		*vbr = true;
		strcpy(bitrate, "0");
	} else if (strcmp(label, GOGO_VBR_1_TXT) == 0) {
		*vbr = true;
		strcpy(bitrate, "1");
	} else if (strcmp(label, GOGO_VBR_2_TXT) == 0) {
		*vbr = true;
		strcpy(bitrate, "2");
	} else if (strcmp(label, GOGO_VBR_3_TXT) == 0) {
		*vbr = true;
		strcpy(bitrate, "3");
	} else if (strcmp(label, GOGO_VBR_4_TXT) == 0) {
		*vbr = true;
		strcpy(bitrate, "4");
	} else if (strcmp(label, GOGO_VBR_5_TXT) == 0) {
		*vbr = true;
		strcpy(bitrate, "5");
	} else if (strcmp(label, GOGO_VBR_6_TXT) == 0) {
		*vbr = true;
		strcpy(bitrate, "6");
	} else if (strcmp(label, GOGO_VBR_7_TXT) == 0) {
		*vbr = true;
		strcpy(bitrate, "7");
	} else if (strcmp(label, GOGO_VBR_8_TXT) == 0) {
		*vbr = true;
		strcpy(bitrate, "8");
	} else if (strcmp(label, GOGO_VBR_9_TXT) == 0) {
		*vbr = true;
		strcpy(bitrate, "9");
	}

	return B_OK;
}

int32
GoGoEncoder::GetFormat(char* format) {
	PRINT(("GoGoEncoder::GetFormat(char*)\n"));

	BMenuItem* item;
	BMenu* formatMenu;

	item = menu->FindItem(GOGO_OUTPUT_FORMAT_STR);
	if (!item) {
		return B_ERROR;
	}
	formatMenu = item->Submenu();
	if (!formatMenu) {
		return B_ERROR;
	}

	item = formatMenu->FindMarked();
	if (!item) {
		return B_ERROR;
	}

	const char* label = item->Label();

	if (strcmp(label, GOGO_STEREO_TXT) == 0) {
		strcpy(format, STEREO_CODE);
	} else if (strcmp(label, GOGO_MONO_TXT) == 0) {
		strcpy(format, MONO_CODE);
	} else if (strcmp(label, GOGO_JSTEREO_TXT) == 0) {
		strcpy(format, JSTEREO_CODE);
	}

	return B_OK;
}

int32
GoGoEncoder::GetPsycho(bool* psycho) {
	PRINT(("GoGoEncoder::GetPsycho(bool*)\n"));

	BMenuItem* item;

	item = menu->FindItem(GOGO_PSYCHO_ACOUSTICS_STR);
	if (!item) {
		return B_ERROR;
	}

	*psycho = item->IsMarked();

	return B_OK;
}

int32
GoGoEncoder::UpdateStatus(FILE* out, BMessenger* messenger) {
	PRINT(("GoGoEncoder::UpdateStatus(FILE*,BMessenger*)\n"));

	float prev = 0.0;
	float curr = 0.0;
	char buffer[2048];
	unsigned char c = 0;
	int i = 0;

	while (1) {
		if (CheckForCancel()) {
			PRINT(("Cancel Requested.\n"));
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

		c = fgetc(out);

		if (c != 0 && c != 13) {
			if (c == '{') {
				i = 0;
			} else if (c == '%') {
				buffer[i] = '\0';
		int start = i;
		while (start > 0 && (isdigit((unsigned char) buffer[start - 1])
				|| buffer[start - 1] == '.')) {
					start--;
				}
				char* tmp = &(buffer[start]);
				curr = atof(tmp);
				float delta = curr - prev;
				prev = curr;
				BMessage updateMessage(B_UPDATE_STATUS_BAR);
				updateMessage.AddFloat("delta", delta);
				messenger->SendMessage(&updateMessage);
				if (curr > 99) {
					break;
				}
			}
			buffer[i] = c;
			i++;
		} else if (c == 13) {
			buffer[i] = 0;
			PRINT(("%s\n", buffer));
			i = 0;
		}

		if (c == 255) {
			break;
		}
	}
	PRINT(("DONE ENCODING\n"));
	return B_OK;
}

int32
GoGoEncoder::WriteDetails(BMessage* message) {
	PRINT(("GoGoEncoder::WriteDetails(BMessage*)\n"));

	const char* outputFile;
	const char* artist;
	const char* album;
	const char* title;
	const char* year;
	const char* comment;
	const char* track;
	const char* genre;

	if (message->FindString("output file", &outputFile) != B_OK) {
		return B_ERROR;
	}

	message->FindString("artist", &artist);
	message->FindString("album", &album);
	message->FindString("title", &title);
	message->FindString("year", &year);
	message->FindString("comment", &comment);
	message->FindString("track", &track);
	message->FindString("genre", &genre);

	dev_t device = dev_for_path(outputFile);
	BVolume volume(device);
	if (volume.InitCheck() != B_OK) {
		return B_ERROR;
	}


	BFile mp3File(outputFile, B_READ_WRITE);
	if (mp3File.InitCheck() != B_OK) {
		return B_ERROR;
	}

	if (volume.KnowsMime()) {
		BNodeInfo info(&mp3File);
		if (info.InitCheck() != B_OK) {
			return B_ERROR;
		}
		if (info.SetType(MP3_MIME_TYPE) != B_OK) {
			return B_ERROR;
		}
	}

	if (volume.KnowsAttr()) {
		AudioAttributes attributes(&mp3File);
		attributes.SetArtist(artist);
		attributes.SetAlbum(album);
		attributes.SetTitle(title);
		attributes.SetYear(year);
		attributes.SetComment(comment);
		attributes.SetTrack(track);
		attributes.SetGenre(genre);
		if (attributes.Write() != B_OK) {
			return B_ERROR;
		}
	}

	ID3Tags tags(outputFile);
	tags.SetArtist(artist);
	tags.SetAlbum(album);
	tags.SetTitle(title);
	tags.SetYear(year);
	tags.SetComment(comment);
	tags.SetTrack(track);
	tags.SetGenre(genre);
	if (tags.Write() != B_OK) {
		return B_ERROR;
	}

	return B_OK;
}

thread_id
GoGoEncoder::CommandIO(int* filedes, int argc, const char** argv) {
	PRINT(("GoGoEncoder::CommandIO(int*,int,const char**)\n"));

	int oldstderr;

	pipe(filedes);
	oldstderr = dup(STDERR_FILENO);
	close(STDERR_FILENO);
	dup2(filedes[1], STDERR_FILENO);

	thread_id ret = load_image(argc, argv, (const char**)environ);

	dup2(oldstderr, STDERR_FILENO);
	close(oldstderr);

	return ret;
}

//function called by Flipside A.E. to get new AEEncoder subclass
AEEncoder*
load_encoder() {
	return new GoGoEncoder();
}
