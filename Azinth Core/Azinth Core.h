#pragma once

#include "framework.h"
#include "resource.h"

// Structure of a single record that will be saved in the decisionBuffer
struct DecisionRecord
{
	// Information about the keypress that generated this record
	RAWKEYBOARD keyboardInput;

	// Information about the action to be taken, if any
	Azinth::PKeystrokeCommand mappedAction;

	// TRUE - this keypress should be blocked, and mappedAction should be carried out
	// FALSE - this keypress should not be blocked, and there is no mapped input to be carried out
	BOOL decision;

	DecisionRecord(RAWKEYBOARD _keyboardInput, BOOL _decision)
		: keyboardInput(_keyboardInput), mappedAction(nullptr), decision(_decision)
	{
		// Constructor
	}

	DecisionRecord(RAWKEYBOARD _keyboardInput, Azinth::PKeystrokeCommand _mappedInput, BOOL _decision)
		: keyboardInput(_keyboardInput), mappedAction(_mappedInput), decision(_decision)
	{
		// Constructor
	}
};

