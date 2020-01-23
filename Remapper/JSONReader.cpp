// Author: Rafael Kenji Takahashi, FATEC Mogi das Cruzes

#include "pch.h"

#include "Remapper.h"
#include "Keyboard.h"
#include "Layer.h"
#include "Scancode.h"
#include "KeystrokeCommands.h"

#include <stdexcept>
#include <algorithm>	// for string replacement


using namespace Azinth;



namespace Azinth
{
	bool Remapper::loadSettings(const std::wstring filename)
	{
		// Mock implementation

		unsigned int keyboardCount = 1;

		// Make a vector for modifier combination
		std::vector<std::wstring> modifierCombination;
		// We'll keep this vector empty

		// Make a list of remaps
		std::unordered_map<Scancode, BaseKeystrokeCommand*> layout;

		// Add one remap to it
		// 0x2a to send the letter A (65)
		auto codepoints = std::vector<unsigned int>();
		codepoints.push_back(65);
		BaseKeystrokeCommand* command = new UnicodeCommand(codepoints, false);
		layout[Scancode(0x2a)] = command;

		// I won't worry about freeing memory because this is just a mock implementation

		// Instantiate a layer
		auto layer = new Layer(modifierCombination, layout);
		auto layers = std::vector<Layer*>();
		layers.push_back(layer);
		// Instantiate a modifier map (with an empty map of modifiers)
		auto modMap = new ModifierStateMap(std::vector<PModifier>());

		// Empty string for keyboard name will match any input
		auto keyboard = new Keyboard(L"", layers, modMap);

		this->keyboards.clear();
		this->keyboards.push_back(keyboard);


		return true;
	}
}


