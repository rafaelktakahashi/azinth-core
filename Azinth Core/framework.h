// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// Additional headers
#include <deque>				// double-ended queue for holding decision records
#include <string>				// std::string and std::wstring
#include <vector>				// contiguous, iterable containers for keyboard structures
#include <map>					// maps for dead keys
#include <unordered_map>		// hash maps for storing the set of remaps for each keyboard
#include <fstream>				// for reading the configuration file
#include <Windows.h>			// for the Windows API
#include <locale>				// setting locale
#include <codecvt>				// for converting strings between different encodings
#include <cctype>				// make sure things like hex digit checking will work (that's also in locale)
#include <shellapi.h>			// to get arguments passed to main

#include "../Remapper/RemapperAPI.h" // Remapper's static library
#include "../KeyboardHookDLL/KeyboardHook.h" // Keyboard Hook DLL, which must be a separate dll