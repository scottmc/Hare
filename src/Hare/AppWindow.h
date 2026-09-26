/*
 * Copyright 2000-2021, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __APP_WINDOW_H__
#define __APP_WINDOW_H__

#include <Window.h>

class AppView;
class BMessage;
class BMessenger;
class BMenu;
class BMenuBar;
class BVolumeRoster;

class AppWindow : public BWindow {
public:
	AppWindow();
	~AppWindow();
	static AppWindow* GetInstance();
	BMenuBar* MenuBar();
	virtual void AboutRequested();
	virtual void FrameResized(float width, float height);
	virtual void MessageReceived(BMessage* message);
	virtual void RefsReceived(BMessage* message);
	virtual bool QuitRequested();
private:
	void InitWindow();
	void InitMenus();
	void LoadCDMenu();
	void LoadEncoderMenu();
	void CheckHaikuRevision();
	void MenuItemSelected(BMessage* message);
	void AddVolumeToList(const char* name);
	void AddVolumeToList(dev_t device);
	void LoadFirstMountedCd();
	AppView* appView;
	BMenuBar* menuBar;
	BMenu* loadCdMenu;
	BMenu* encoderMenu;
	BMessenger* windowMessenger;
	BMessenger* viewMessenger;
	BVolumeRoster* volumes;
	image_id encoderAddon;
	// Set by CheckHaikuRevision() at startup - false when the running
	// Haiku predates the SCSI CD driver fixes needed to load a CD
	// safely (see CheckHaikuRevision()'s own comment in AppWindow.cpp).
	// LoadCDMenu() and AddVolumeToList() both honor this so the user
	// can't get far enough to hit the KDL it guards against.
	bool fCdOperationsAllowed;
};

#endif
