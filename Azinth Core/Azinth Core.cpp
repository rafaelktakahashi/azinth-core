// Azinth Core.cpp : Defines the entry point for the application.
//
// Rafael Kenji Takahashi
// Some parts of this file were originally written by V�t Blecha.

#include "framework.h"
#include "Azinth Core.h"
#include "Scancodes.h"

#define MAX_LOADSTRING 100
#if _DEBUG
#define DEBUG_TEXT_SIZE	128
#endif

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Handle to the main executable window
HWND mainHwnd;
// Windows message for communication between main executable and DLL module
UINT const WM_HOOK = WM_APP + 1;
// How long should hook processing wait for the matching Raw INput message, in ms
DWORD maxWaitingTime = 50;

// Flag for the AltGr fix
BOOL AltGrBlockNextLCtrl = FALSE;

// Buffer for keyboard Raw Input struct
UINT rawKeyboardBufferSize = 32;
LPBYTE rawKeyboardBuffer = new BYTE[rawKeyboardBufferSize];

// Buffer for keyboard name
UINT keyboardNameBufferSize = 128;
WCHAR* keyboardNameBuffer = new WCHAR[keyboardNameBufferSize];

// Structure to hold a Raw Input pointer
RAWINPUT* raw;

// Structure used in the function ResetKey
INPUT* input;

Azinth::PRemapper remapper; // PRemapper is a pointer

// Text to display on screen for debugging
#if _DEBUG
WCHAR* debugText = new WCHAR[128];
WCHAR* debugTextKeyboardName = new WCHAR[128];
WCHAR* debugTextBeingBlocked = new WCHAR[128];
#endif

// Variables to hold timer values
DWORD currentTime, startTime;

// Buffer for the decisions for whether to block the input with Hook
std::deque<DecisionRecord> decisionBuffer;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_AZINTHCORE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    // There are no accelerators in this program
    // HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AZINTHCORE));

    MSG msg;

    // Evaluate arguments (path to configuration file)
    LPWSTR* szArgList; // 0: name of exe, 1: path to config file
    int argCount;
    szArgList = CommandLineToArgvW(GetCommandLineW(), &argCount);

	Azinth::Create(&remapper);

    if (szArgList == NULL)
    {
#if _DEBUG
        OutputDebugString(L"No arguments received.");
#endif
        remapper->loadSettings(L"Placeholder");
    }
    else if (argCount != 2)
    {
#if _DEBUG
        OutputDebugString(L"Incorrect number of arguments. Initializing with default file");
#endif
        remapper->loadSettings(L"Placeholder");
    }
    else
    {
        // Try to initialize the remapper
        if (!remapper->loadSettings(std::wstring(szArgList[1])))
        {
#if _DEBUG
            OutputDebugString(L"Failed to open file. Initializing with default file.");
#endif
            remapper->loadSettings(L"Placeholder");
        }
    }

    LocalFree(szArgList);

    // Initialize the work INPUT structure for use in the ResetKey function
    input = new INPUT;
    input->type = INPUT_KEYBOARD;
    input->ki.dwExtraInfo = 0;
    input->ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
    input->ki.wScan = 0;
    input->ki.time = 0;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        // if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        // {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        // }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance) // receives a handle to an application instance
{
    WNDCLASSEXW wcex = { };

    wcex.cbSize = sizeof(WNDCLASSEX);

    // wcex.style          = CS_HREDRAW | CS_VREDRAW;
    // Pointer to the window procedure
    wcex.lpfnWndProc    = WndProc;
    // wcex.cbClsExtra     = 0;
    // wcex.cbWndExtra     = 0;
    // Handle to the application instance
    wcex.hInstance      = hInstance;
    // wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AZINTHCORE));
    // wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    // wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    // wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_AZINTHCORE);
    wcex.lpszClassName  = szWindowClass;
    // wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // Store the handle globally
   mainHwnd = hWnd;

   // Do not show the window; this program runs in the background.
   // ShowWindow(hWnd, nCmdShow);
   // UpdateWindow(hWnd);

   // Register for receiving Raw Input for keyboards
   RAWINPUTDEVICE rawInputDevice[1];
   rawInputDevice[0].usUsagePage = 1; // usage page 1 is generic and usage 6 is for keyboards
   rawInputDevice[0].usUsage = 6; // (2 is mouse, 4 is joystick, there are others)
   rawInputDevice[0].dwFlags = RIDEV_INPUTSINK; // Receive input even if the registered window is in the background
   rawInputDevice[0].hwndTarget = hWnd; // Handle to the target window (NULL makes it follow keyboard focus)
   RegisterRawInputDevices(rawInputDevice, 1, sizeof(rawInputDevice[0]));

   // Setup the keyboard hook (from the dll)
   InstallHook(hWnd);
   // The hook needs to be executed from a separate dll because it's a global hook.

   return TRUE;
}

// Simulates an up keystroke of the specified key.
// Mostly useful for resetting the alt key.
BOOL ResetKey(SHORT vKey)
{
    // Input is already initialized for sending a release keystroke.
    input->ki.wVk = vKey;
    return SendInput(1, input, sizeof(INPUT));
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        // Raw INput Message:
    case WM_INPUT:
    { // brackets for locality
        UINT bufferSize = 0; // work variable

        // Get data from the raw input structure
        // Parameters:
        // 1. HRAWINPUT - a handle to the raw input structure (in this case stored in the long param of the message)
        // 2. UINT - a flag containing what to return, either RID_HEADER (header) or RID_INPUT (raw data)
        // 3. LPVOID, out param - a pointer to the data that comes from the RAW INPUT structure
        //		If this is null, the next parameter will contain the required size of the buffer
        // 4. PUINT, in or out param - size of the data, or required size if previous param is NULL
        // 5. UINT - size in bytes of the header

        // Get required buffer size, check if it's larger than what we have
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));
        if (bufferSize > rawKeyboardBufferSize)
        {
            // Needs more space than we have; let's replace our buffer with a bigger one.
            rawKeyboardBufferSize = bufferSize;
            delete[] rawKeyboardBuffer;
            rawKeyboardBuffer = new BYTE[rawKeyboardBufferSize];
#if _DEBUG
            OutputDebugString(L"Needed more space for keyboard buffer");
#endif
        }

        // Load data into buffer
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawKeyboardBuffer, &rawKeyboardBufferSize, sizeof(RAWINPUTHEADER));

        // Cast the contents of the buffer into our rawinput buffer
        raw = (RAWINPUT*)rawKeyboardBuffer;

#if _DEBUG
        WCHAR* text = new WCHAR[128];
        swprintf_s(text, 128, L"Raw Input: Virtual key %X scancode %s%s%X (%s)\n",
            raw->data.keyboard.VKey,		// virtual keycode
            (raw->data.keyboard.Flags & RI_KEY_E0 ? L"e0 " : L""),
            (raw->data.keyboard.Flags & RI_KEY_E1 ? L"e1 " : L""),
            raw->data.keyboard.MakeCode,	// scancode
            raw->data.keyboard.Flags & RI_KEY_BREAK ? L"up" : L"down");		// keydown or keyup (make/break)
        OutputDebugString(text);
        memcpy_s(debugText, DEBUG_TEXT_SIZE, text, 128);	// will redraw later
        delete[] text;
#endif

        // Get the device name
        // But first check that our buffer is large enough
        GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICENAME, NULL, &bufferSize);
        if (bufferSize > keyboardNameBufferSize)		// It needs more space than we have!
        {
            keyboardNameBufferSize = bufferSize;
            delete[] keyboardNameBuffer;
            keyboardNameBuffer = new WCHAR[keyboardNameBufferSize];
#if _DEBUG
            OutputDebugString(L"Needed more space for device name buffer");
#endif
        }

        // Load the device name into the buffer
        GetRawInputDeviceInfo(
            raw->header.hDevice,
            RIDI_DEVICENAME,
            keyboardNameBuffer,
            &keyboardNameBufferSize
        );

        /*----Fix for Fake shift----*/
        // When shift + numpad key is pressed while numlock is on, a fake shift up is pressed on keydown,
        // and a fake shift down is pressed on keyup to simulate unshifted behavior (similar to capslock
        // and shift, but that doesn't cause fake shifts).
        // We look for a virtual-key code translated from a scancode that normally wouldn't produce it.
        if (raw->data.keyboard.VKey == VK_SHIFT		// (legit) shift, either side
            && raw->data.keyboard.MakeCode != SCANCODE_LEFT_SHIFT	// scancode is not left shift
            && raw->data.keyboard.MakeCode != SCANCODE_RIGHT_SHIFT)	// and is not right shift
        {
            // then it must be an illegitimate shift, not produced by a physical keystroke
            // but there is a Hook message for a legitimate (although faked) shift waiting for it
            // Instead of storing the decision for this keystroke, store the decision for both a
            // left shift and a right shift, since we don't know which one produced this message
            OutputDebugString(L"Raw Input: Fake shift detected, storing two shift decisions.\n");
            Azinth::PKeystrokeCommand possibleAction;

            // keyup and keydown is wrong
            if (raw->data.keyboard.Flags & RI_KEY_BREAK)
                raw->data.keyboard.Flags &= 0xfffe;		// unset last bit
            else raw->data.keyboard.Flags |= RI_KEY_BREAK;	// set last bit

                                                            // pretend this is a left shift
            raw->data.keyboard.MakeCode = 0x2a;
            bool DoBlock = remapper->evaluateKey(&(raw->data.keyboard), keyboardNameBuffer, &possibleAction);
            decisionBuffer.push_back(DecisionRecord(raw->data.keyboard, possibleAction, DoBlock));	// remember the answer

                                                                                                    // pretend this is a right shift
            raw->data.keyboard.MakeCode = 0x36;
            DoBlock = remapper->evaluateKey(&(raw->data.keyboard), keyboardNameBuffer, &possibleAction);		// ask
            decisionBuffer.push_back(DecisionRecord(raw->data.keyboard, possibleAction, DoBlock));	// remember the answer

            return 0;
    }
        // This fix prevents most timeouts, but occasional lag and wrong behavior still occur.
        // There is a copy of this on the delayed message loop.
        /*---End of fix for Fake shift----*/

#if _DEBUG
        memcpy_s(debugTextKeyboardName, DEBUG_TEXT_SIZE, keyboardNameBuffer, keyboardNameBufferSize);
        text = new WCHAR[200];
        swprintf_s(text, 200, L"Raw Input: Keyboard name is %ls\n", keyboardNameBuffer);
        OutputDebugString(text);
        delete[] text;
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
#endif

        // Call the function that devices whether to block or allow this keystroke
        // Store that decision in the decisionBuffer; look for it when the hook asks for it.

        // Check whether to block this key, and store the decision for when the hook asks for it
        Azinth::PKeystrokeCommand possibleAction = nullptr; // we don't know yet if our key maps to anything
        BOOL doBlock = remapper->evaluateKey(&(raw->data.keyboard), keyboardNameBuffer, &possibleAction);

#if _DEBUG
        if (doBlock)
            OutputDebugString(L"Raw Input: This key should be blocked. Recording it in the decision record.\n");
        else
            OutputDebugString(L"Raw Input: This key should not be blocked, and will not be recorded.\n");
#endif

        // Remeber the decision.
        decisionBuffer.push_back(DecisionRecord(raw->data.keyboard, possibleAction, doBlock));

        // Special handling for the Alt keys:
        // If the received key was either a Left Alt or a Right Alt that was blocked then the state
        // of the keyboard has to be reset. This prevents subsequent simulated keystrokes from being
        // sent with the Alt modifier.
        if ((raw->data.keyboard.MakeCode == SCANCODE_ALT && doBlock))
        {
#if _DEBUG
            OutputDebugString(L"Raw Input: Got a blocked Alt; resetting keyboard state.\n");
#endif
            if (raw->data.keyboard.Flags & RI_KEY_E0)	// <- if it's RAlt
                ResetKey(VK_RMENU);
            else
                ResetKey(VK_LMENU);
        }

        return 0; // exit Wndproc
    } // end of case WM_INPUT

    case WM_HOOK: // <- message defined by this program
    {
        // Message from Hooking DLL
        // It means we need to look for a corresponding Raw Input message that should have arrived before
        // That message is the one that can tell us whether or not to block the key,
        // and this message is the one that can block the key.
        // (or maybe it didn't arrive yet. Checking is necessary.)

		// In this message, word parameter contains the virtual key code (not scancode),
		// and long parameter contains whether or not the keypress is a down key.
		USHORT virtualKeyCode = (USHORT)wParam;

		// USHORT repeatCount = (lParam & 0xffff);		// extract the two last bytes
		// Repeat count doesn't increment when user holds down a key
		// which is weird; I suspect it only increments when messages fail to be sent in time
		// which is a lot more worrying

		// Flags: bits 16 to 23 in lParam contains the scancode, 24 indicates extended key, bit 29 indicates the ALT key, 31 is keyup
		// Don't forget to count the bits from 0, least significant to most significant.
		USHORT keyPressed = !((lParam >> 31) & 1);			// gets the thirty-first bit which is 1 when it's a keyup; invert it
		USHORT extractedScancode = (lParam >> 16) & 255;	// extract one full byte at position 16
		USHORT isExtended = (lParam >> 24) & 1;				// twenty-fourth bit
															// There exists a KF_ALTDOWN define set to 0x2000. Very sadly, that's incorrect and points to bit 13 instead of 29.
															// In fact, all of these KF flags seem to be one byte less than they should. I don't know the deal with that.
		USHORT isAltKeyDown = (lParam >> 29) & 1;			// twenty-ninth bit


															// We should ignore keys that go up when they were already up. For now, we send a warning to the debug screen.
		USHORT previousStateFlagWasDown = (lParam >> 30) & 1;




#if _DEBUG
		WCHAR text[128];
		swprintf_s(text, 128, L"Hook: vKey=%X keydown=%d sc=%x extend=%d Alt=%d raw lParam=%x\n",
			virtualKeyCode, keyPressed, extractedScancode, isExtended, isAltKeyDown, lParam);
		OutputDebugString(text);	// <- sends to debug window

		if (!previousStateFlagWasDown && !keyPressed)
		{
			WCHAR text[128];
			swprintf_s(text, 128, L"Hook: Redundant keyup vkey=%X sc=%x ext=%d\n",
				virtualKeyCode, extractedScancode, isExtended);
			OutputDebugString(text);

		}
#endif


		/*----Dealing with Pause/Break----*/
		// In case this is the hook message generated by the Pause/Break key (which generates two Raw Input messages),
		// look for (virtual key = 0x13, scancode = (e1) 1d) instead. Do not wait for the second Raw Input message,
		// which should be intercepted somewhere else anyway.
		if (virtualKeyCode == 0x13		// <- vKey for Pause/Break
			&& extractedScancode == 0x45)		// <- sc for NumLock
		{
			// Is pause/break
			extractedScancode = 0x1d;		// Will wait for (virtual key = 0x13, scancode = 1d) instead.
											// That's why we don't declare everything const. Things change here.
#if _DEBUG
			OutputDebugString(L"Hook: Received a Pause/Break, will look for raw vkey13 sc=1d\n");
#endif
		}
		// It's not necessary to look for the second Raw Input message (vkey = 0xff, sc = 0x45), since no Hook message will find it.
		// That would only add an extra check statement. Just make sure that any remaps for Pause/Break are set to activate
		// upon receiving scancode 0xe1 0x1d instead of 0x45.
		/*----Finished dealing with Pause/break----*/



		/*----Dealing with PrintScreen----*/
		// normally only sends a single keyup
		if (virtualKeyCode == VK_SNAPSHOT		// printscreen
			&& ((extractedScancode == SCANCODE_PRINT_SCREEN		// scancode for prtscn alone or with shift, or with ctrl
				&& isExtended == 1)
				|| (extractedScancode == SCANCODE_SYS_REQ		// scancode for prtscn with alt
					&& isExtended == 0))
			&& keyPressed == 0)		// key up
		{
			// we send a keypress down (keyboard name shouldn't matter)
			LPARAM mockLParam = lParam;
			mockLParam &= 0x7fffffff;		// set thirty-first bit to 0 (flag for keypress up)

#if _DEBUG
			OutputDebugString(L"Hook: Up PrintScreen received, will send fake keypress down to self\n");
#endif

			// This should interrupt processing
			SendMessage(mainHwnd, WM_HOOK, wParam, mockLParam);
			// Output value goes nowhere, because there is no real key to be blocked
		}
#if _DEBUG
		else if (virtualKeyCode == VK_SNAPSHOT
			&& ((extractedScancode == 0x37
				&& isExtended == 1)
				|| (extractedScancode == 0x54
					&& isExtended == 0))
			&& keyPressed == 1)		// key down
			OutputDebugString(L"Hook: Received message from self, will look for PrintScreen down before up\n");
#endif
		/*----Finished dealing with PrintScreen----*/


		// Check the Raw Input buffer to see if this Hook message is supposed to be blocked; this WdnProc returns 1 if it is
		bool blockThisHook = false;
		bool recordFound = false;
		if (!decisionBuffer.empty())
		{
			int index = 0;
			// Search the buffer for the matching record (it should exist)
			for (std::deque<DecisionRecord>::iterator iterator = decisionBuffer.begin();
				iterator != decisionBuffer.end();
				iterator++, index++)
			{
				if (iterator->keyboardInput.VKey == virtualKeyCode
					&& iterator->keyboardInput.MakeCode == extractedScancode
					&& !(iterator->keyboardInput.Flags & RI_KEY_BREAK) == keyPressed
					&& ((iterator->keyboardInput.Flags & RI_KEY_E0) == RI_KEY_E0) == (isExtended > 0))		// match!
				{
					// Actually, this doesn't guarantee a match;
					// Keys in two different keyboards corresponding to the same virtual key may be pressed in rapid succession
					// We have to assume that people don't do that normally.

#if _DEBUG
					if (iterator->decision) OutputDebugString(L"Hook: Must block this key.\n");
					else if (iterator->decision == FALSE) OutputDebugString(L"Hook: Must let this key through.\n");
#endif


					// Now, if the decision was to block the hook, we must act on it at this point, just before popping it
					if (iterator->decision) {

						if (!iterator->mappedAction->execute(!keyPressed, previousStateFlagWasDown && keyPressed)) {
#if _DEBUG
							OutputDebugString(L"Simulation failed!!\n");
#endif
						}
					}

					recordFound = TRUE;		// set the flags
					blockThisHook = iterator->decision;

					// Then, remove this and all preceding messages from the buffer
					for (int i = 0; i <= index; i++)		// <- up until and including this one
						decisionBuffer.pop_front();

					// We found the item, so we break the for loop to stop looking:
					break;

				} // end if match

			} // end for search

		} // end if buffer.empty

		  // At this point, recordFound is either TRUE or FALSE, and if TRUE, blockThisHook has the correct choice.

		  // Wait for the matching Raw Input message if the decision buffer was empty or the matching record wasn't there


		  // Variables used for timers are startTime and currentTime
		startTime = GetTickCount64();			// <- record start time

											// Will only fall into this if we didn't find the correct record.
											// It's a lot of work, but hopefully won't happen frequently.
		while (!recordFound)
		{
			MSG rawMessage;		// We're peeking the message; that is, we're trusting that the very next raw input message
								// will be the one we're waiting for.
								// Also, we can't wait for the other case in this switch to be called, because we'd need
								// to interrupt this case.
			while (!PeekMessage(&rawMessage, mainHwnd, WM_INPUT, WM_INPUT, PM_REMOVE))		// this does not remove the message
			{
				// Message didn't arrive!

				// Test for the maxWaitingTime
				currentTime = GetTickCount64();
				// If current time is less than start, the time rolled over to 0
				if ((currentTime < startTime ? ULONG_MAX - startTime + currentTime : currentTime - startTime) > maxWaitingTime)
				{
					// Ignore the Hook message if it exceeded the limit
#if _DEBUG
					WCHAR text[128];
					swprintf_s(text, 128, L"Hook timed out: %X (%d)\n", virtualKeyCode, keyPressed);
					OutputDebugString(text);
#endif
					return 0;
				}

				// Take a nap until checking again.
				Sleep(10);			// Take this out if it causes problems.
			}

			// The Raw Input message has arrived; decide whether to block the input
			// We're still in that case that the raw message took long to arrive.
			UINT bufferSize;

			// See if we'll need more space
			GetRawInputData((HRAWINPUT)rawMessage.lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));
			if (bufferSize > rawKeyboardBufferSize)
			{
				// Need more space!
				rawKeyboardBufferSize = bufferSize;
				delete[] rawKeyboardBuffer;
				rawKeyboardBuffer = new BYTE[rawKeyboardBufferSize];
#if _DEBUG
				OutputDebugString(L"Needed more space for the delayed raw input message.");
#endif
			}
			// Load data into the buffer
			GetRawInputData((HRAWINPUT)rawMessage.lParam, RID_INPUT, rawKeyboardBuffer, &rawKeyboardBufferSize, sizeof(RAWINPUTHEADER));

			RAWINPUT* raw = (RAWINPUT*)rawKeyboardBuffer;		// <- casting into correct type for convenience



																// A lot of code here is similar to that in the WM_INPUT case.
#if _DEBUG
			WCHAR text[128];		// <- unnecessary allocation, but this is just for debug
			swprintf_s(text, 128, L"(delayed) Raw Input: virtual key %X scancode %s%s%X (%s)\n",
				raw->data.keyboard.VKey,		// virtual keycode
				raw->data.keyboard.Flags & RI_KEY_E0 ? L"e0 " : L"",
				raw->data.keyboard.Flags & RI_KEY_E1 ? L"e1 " : L"",
				raw->data.keyboard.MakeCode,
				raw->data.keyboard.Flags & RI_KEY_BREAK ? L"up" : L"down");		// keydown or keyup (make/break)
			OutputDebugString(text);
			memcpy_s(debugText, DEBUG_TEXT_SIZE, text, 128);	// will redraw later
#endif

																// Prepare string buffer for the device name
			GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICENAME, NULL, &bufferSize);
			if (bufferSize > keyboardNameBufferSize)
			{
				// Need more space than we have!
				keyboardNameBufferSize = bufferSize;
				delete[] keyboardNameBuffer;
				keyboardNameBuffer = new WCHAR[keyboardNameBufferSize];
#if _DEBUG
				OutputDebugString(L"Needed more space for keyboard name in the delayed raw input message");
#endif
			}
			// Load the device name into the buffer
			GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICENAME, keyboardNameBuffer, &keyboardNameBufferSize);

			/*----Fix for fake shift----*/
			// There is another copy of this on the Raw Input case, for when the message arrives in time.
			// In there, we don't know which version of Shift (left or right) produced the raw message
			// But if we're in here, we have that information. We check differently:
			// 1. If this message is either shift:
			// 2.		If the delayed raw input is a shift from a scancode that shouldn't produce it
			// 3.				Then we change that message's scancode (and flip the keyup flag, which is wrong)
			// 4. Continue normally.
			if (virtualKeyCode == 0x10		// This is a shift
				&& (extractedScancode == 0x2a || extractedScancode == 0x36))	// yep. shift.
			{
				if (raw->data.keyboard.VKey == VK_SHIFT				// delayed signal is a shift
					&& raw->data.keyboard.MakeCode != SCANCODE_LEFT_SHIFT		// but not produced by left shift physical key
					&& raw->data.keyboard.MakeCode != SCANCODE_RIGHT_SHIFT)		// nor right shift physical key
				{
					// keyup flag will be wrong; invert it.
					if (raw->data.keyboard.Flags & RI_KEY_BREAK)
						raw->data.keyboard.Flags &= 0xfffe;		// unset last bit
					else raw->data.keyboard.Flags |= RI_KEY_BREAK;	// set last bit

					raw->data.keyboard.MakeCode = extractedScancode;				// *You're the same as us*
																					// That'll make this Hook continue, and recognize this delayed message as its match.
				}
			}
			// (I wish we could fake RawInput messages. Doesn't seem like so).
			/*---End of fix for Fake shift----*/



			// Evaluate and take action depending on whether the message is the one we were expecting:

			// If the raw input message doesn't match the hook, push it into the buffer and continue waiting
			if (virtualKeyCode != raw->data.keyboard.VKey
				|| extractedScancode != raw->data.keyboard.MakeCode
				|| keyPressed != (raw->data.keyboard.Flags & RI_KEY_BREAK ? 1 : 0)
				|| (isExtended > 0) != ((raw->data.keyboard.Flags & RI_KEY_E0) == RI_KEY_E0))	// Plenty of checks here
			{
				// Turns out this raw input message wasn't the one we were looking for.
				// Put it in the queue just like we did in the WM_INPUT case, and keep waiting.
				Azinth::PKeystrokeCommand possibleInput;
				BOOL doBlock = remapper->evaluateKey(&(raw->data.keyboard), keyboardNameBuffer, &possibleInput);
				decisionBuffer.push_back(DecisionRecord(raw->data.keyboard, possibleInput, doBlock));




				/*--Checking for AltGr--*/
				// Checking here because it never happens early
				if (extractedScancode == SCANCODE_CONTROL		// Hook message generated by left control (scancode of LCtrl in translated set 2)
					&& isExtended == FALSE			// left variant
					&& virtualKeyCode == VK_CONTROL	// <- Control key (left variant, since isExtended is false)
					&& keyPressed == 1)			// <- down
				{
					if (raw->data.keyboard.MakeCode == SCANCODE_ALT		// <- Raw Input is Alt key
						&& raw->data.keyboard.Flags & RI_KEY_E0)	// Right variant
					{
						// then we won't wait for the LCtrl Raw Input message because it'll never happen.
						AltGrBlockNextLCtrl = TRUE;

						return TRUE;	// <- will block
					}
				}
				// if a LCtrl keydown was blocked because it was part of an AltGr,
				// we should block the very next LCtrl up. Except if another LCtrl down was sent along the way.
				if (extractedScancode == SCANCODE_CONTROL	// scancode of Ctrl
					&& isExtended == FALSE		//		<- left variant of
					&& virtualKeyCode == VK_CONTROL	//	<- Control
					&& keyPressed == 0)				//	<- up
				{
					if (AltGrBlockNextLCtrl)	// after a fake LCtrl down from an AltGr
					{
						AltGrBlockNextLCtrl = FALSE;
						if (extractedScancode == SCANCODE_CONTROL && isExtended == FALSE && keyPressed == 0)
							return TRUE;
					}
				}
				// A Ctrl press while AltGr is down will cause AltGr to lose effect.
				// That's okay because that happens normally. Do not be alarmed.
				// Let's hope all keyboards we find use the translated scancode set 2.
				// Or we'll have to implement support for different scancode sets.
				/*--Finished checking for AltGr--*/
			}
			else
			{
				// This is truly the message we were looking for.
				recordFound = TRUE;		// This will get us out of the loop.
										// (the other way to exit the loop is by timing out)
										// But we still didn't evaluate the raw message (it just arrived!)
				Azinth::PKeystrokeCommand possibleOutput;
				blockThisHook = remapper->evaluateKey(&(raw->data.keyboard), keyboardNameBuffer, &possibleOutput);
				// Immediately act on the input if there is one, since this decision won't be stored in the buffer
				if (blockThisHook) {

					if (!possibleOutput->execute(!keyPressed, previousStateFlagWasDown && keyPressed)) {
#if _DEBUG
						OutputDebugString(L"WndProc: Command failed!!\n");
#endif
					}
				}
			}

		}

#if _DEBUG
		if (blockThisHook)
		{
			WCHAR text[128];
			swprintf_s(text, 128, L"Keyboard event: %X (%d) is being blocked!\n", virtualKeyCode, keyPressed);
			memcpy_s(debugTextBeingBlocked, DEBUG_TEXT_SIZE, text, 128);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
			OutputDebugString(text);
		}
		else
		{
			WCHAR text[128];
			swprintf_s(text, 128, L"Keyboard event: %X (%d) passed through.\n", virtualKeyCode, keyPressed);
			memcpy_s(debugTextBeingBlocked, DEBUG_TEXT_SIZE, text, 128);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
		}
#endif

		return blockThisHook;	// exit WndProc, the message caller receives 1 or 0
								// Message caller is the hook dll. It sends a message to this window (this message) upon receival of a hook signal,
								// and blocks it if this message returns 1.

    }
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        break;
    case WM_DESTROY:
        // TODO: Uninstall hook
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
