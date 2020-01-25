// Author: Rafael Kenji Takahashi, FATEC Mogi das Cruzes

#include "pch.h"

#include "Remapper.h"
#include "Keyboard.h"
#include "Layer.h"
#include "Scancode.h"
#include "KeystrokeCommands.h"

#include <stdexcept>

// for reading files
#include <fstream>

// for converting between utf-8 std::string and utf-16 std::wstring
#include <iostream>
#include <locale>
#include <codecvt>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace Azinth;

// Prototypes for loading parts of the json
Keyboard* readKeyboard(json);
ModifierStateMap* readListOfModifiers(json);
BaseModifier* readModifier(json);
Layer* readLayer(json);

UnicodeCommand* readUnicodeCommand(json);
DeadKeyCommand* readDeadKeyCommand(json);
MacroCommand* readMacroCommand(json);
ExecutableCommand* readExecutableCommand(json);


// Utility function to read a scancode.
// Azinth uses the notation XX (with hex digits) for single-byte scancodes
// and XX:YY for two-byte scancodes.
Scancode scancodeFromString(std::wstring str)
{
	// First, remove all occurrences of ':' from the input
	std::wstring output;
	output.reserve(str.size());
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] != ':') output += str[i];
	}
	// Now we have a hexadecimal of two or four digits.
	unsigned short iScancode = std::stoi(output.c_str(), 0, 16);
	if (iScancode)
	{
		return Scancode(iScancode & 0xFF);
	}
	else
	{
		return Scancode(iScancode >> 8, iScancode & 0xFF);
	}
}


std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
// Utility function to obtain a utf-16 wstring (which is used internally in Azinth)
// from a utf-8 string (used by the json library, and basically everywhere)
std::wstring toUtf16(std::string input)
{
	return converter.from_bytes(input);
}
std::wstring toUtf16(json::value_type input)
{
	std::string asString = input;
	return toUtf16(asString);
}


// Utility function to obtain a vector<unsigned int> from a json array.
std::vector<unsigned int> readCodepoints(json codepointsArray)
{
	auto codepoints = std::vector<unsigned int>();
	for (auto& cp : codepointsArray)
	{
		codepoints.push_back(cp.get<unsigned int>());
	}
	return codepoints;
}



namespace Azinth
{
	bool Remapper::loadSettings(const std::wstring filename)
	{
		std::ifstream ifstream(filename);
		json j;
		ifstream >> j;

		auto jKeyboards = j["keyboards"];
		this->keyboards.clear();
		for (auto& jKeyboard : jKeyboards)
		{
			this->keyboards.push_back(readKeyboard(jKeyboard));
		}

		int* c; // for breakpoint

		return true;
	}
}



Keyboard* readKeyboard(json j)
{
	std::wstring name = toUtf16(j["name"]);
	ModifierStateMap* modifiers = readListOfModifiers(j["modifiers"]);
	auto layers = std::vector<Layer*>();
	for (auto& layer : j["layers"])
	{
		layers.push_back(readLayer(layer));
	}
	return new Keyboard(name, layers, modifiers);
}

ModifierStateMap* readListOfModifiers(json j)
{
	auto pModifiers = std::vector<PModifier>();
	for (auto& modifier : j)
	{
		std::wstring name = toUtf16(modifier["name"]);
		auto scancodes = std::vector<Scancode>();
		for (auto& sc : modifier["scancodes"])
		{
			scancodes.push_back(scancodeFromString(toUtf16(sc)));
		}
		// scancodes now contains all keys that are part of this modifier
		if (scancodes.size() == 1)
		{
			// Add a simple modifier to the vector
			auto newModifier = new SimpleModifier(name, scancodes.at(0));
			pModifiers.push_back(newModifier);
		}
		else
		{
			// Add a composite modifier to the vector
			auto newModifier = new CompositeModifier(name, scancodes);
			pModifiers.push_back(newModifier);
		}
	}
	return new ModifierStateMap(pModifiers);
}

Layer* readLayer(json j)
{
	// Names of all modifiers that need to be *ON* in order to trigger
	// this layer. All other modifiers must be *OFF*.
	auto modifierNamesJson = j["modifiers"];
	auto modifierNames = std::vector<std::wstring>();
	for (auto& modifier : modifierNamesJson)
	{
		modifierNames.push_back(toUtf16(modifier));
	}
	// Read each remap and put it in a map
	// BaseKeystrokeCommand is for internal use in the Remapper; it's exposed
	// as IKeystrokeCommand to the outside.
	auto remaps = std::unordered_map<Scancode, BaseKeystrokeCommand*>();
	for (auto& remap : j["remaps"])
	{
		std::wstring sc = toUtf16(remap["scancode"]);
		std::string type = remap["type"];

		BaseKeystrokeCommand* command = nullptr;
		if (type.compare("unicode") == 0)
		{
			command = readUnicodeCommand(remap);
		}
		else if (type.compare("macro") == 0)
		{
			command = readMacroCommand(remap);
		}
		else if (type.compare("executable") == 0)
		{
			command = readExecutableCommand(remap);
		}
		else if (type.compare("deadkey") == 0)
		{
			command = readDeadKeyCommand(remap);
		}

		remaps[scancodeFromString(sc)] = command;
	}
	return new Layer(modifierNames, remaps);
}

UnicodeCommand* readUnicodeCommand(json j)
{
	auto codepoints = readCodepoints(j["codepoints"]);
	bool triggerOnRepeat = j["triggerOnRepeat"];
	return new UnicodeCommand(codepoints, triggerOnRepeat);
}

MacroCommand* readMacroCommand(json j)
{
	auto keypresses = std::vector<unsigned short>();
	for (auto& vKey : j["keypresses"])
	{
		unsigned short parsedVirtualKey = vKey["vKey"];
		std::string type = vKey["type"];
		if (type.compare("break") == 0)
		{
			// set the high bit
			parsedVirtualKey |= 0x8000;
		}
		keypresses.push_back(parsedVirtualKey);
	}
	bool triggerOnRepeat = j["triggerOnRepeat"];
	return new MacroCommand(keypresses, triggerOnRepeat);
}

ExecutableCommand* readExecutableCommand(json j)
{
	std::wstring commandPath = toUtf16(j["path"]);
	if (j.contains("arguments"))
	{
		std::wstring commandArguments = toUtf16(j["arguments"]);
		return new ExecutableCommand(commandPath, commandArguments);
	}
	else
	{
		return new ExecutableCommand(commandPath);
	}


}

DeadKeyCommand* readDeadKeyCommand(json j)
{
	// Independent codepoints
	auto codepoints = readCodepoints(j["codepoints"]);
	bool triggerOnRepeat = j["triggerOnRepeat"];
	// Replacements; UnicodeCommands are instantiated for the sole purpose
	// of holding 
	auto replacements = std::unordered_map<UnicodeCommand*, UnicodeCommand*>();
	for (auto& repl : j["replacements"])
	{
		auto codepointsFrom = readCodepoints(repl["from"]);
		auto codepointsTo = readCodepoints(repl["to"]);
		UnicodeCommand* commandFrom = new UnicodeCommand(codepointsFrom, false);
		UnicodeCommand* commandTo = new UnicodeCommand(codepointsTo, false);
		replacements[commandFrom] = commandTo;
	}
	return new DeadKeyCommand(codepoints, replacements, triggerOnRepeat);
}