/*
 * Copyright 2000-2021, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PREF_WINDOW_H
#define PREF_WINDOW_H

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

#include "Settings.h"

class PrefWindow : public BWindow {
public:
	PrefWindow();
	~PrefWindow();
	virtual void MessageReceived(BMessage* message);
private:
	void InitWindow();
	// The Tagging and Attributes box: checkboxes that take effect (and are
	// saved) as soon as they're clicked.
	BBox* InitTaggingBox();
	BCheckBox* AddOption(pref_option option, const char* label);
	// Greys out the options that depend on one that's off.
	void UpdateOptionsEnabled();
	BBox* fileNamePatternBox;
	BStringView* artistStringView;
	BStringView* albumStringView;
	BStringView* titleStringView;
	BStringView* yearStringView;
	BStringView* commentStringView;
	BStringView* trackStringView;
	BStringView* genreStringView;
	BTextControl* fileNamePatternTextControl;
	BButton* applyButton;
	BButton* revertButton;
	BButton* encoderPatternButton;
	BCheckBox* optionBoxes[PREF_OPTION_COUNT];
	BMenuField* coverFormatField;
};

#endif // PREF_WINDOW_H
