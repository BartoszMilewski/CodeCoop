// ---------------------------
// (c) Reliable Software, 2004
// ---------------------------

// Global hooks for mouse and keyboard messages

#include "precompiled.h"
#include "GlobalHooks.h"
#include <Win/Instance.h>

#pragma data_seg(".shared")
unsigned int lastMouseEventTime = 0;
unsigned int lastKeyboardEventTime = 0;
#pragma data_seg()
// set attributes for read-write access and share this section among all processes that load the dll
#pragma comment(linker, "/section:.shared,rws")

Win::Instance DllInstance;
HHOOK mouseHook;
HHOOK keyboardHook;

static LRESULT CALLBACK MouseHookProc (UINT code, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK KeyboardHookProc (UINT code, WPARAM wParam, LPARAM lParam);

BOOL APIENTRY DllMain (HINSTANCE hInstance, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		DllInstance = hInstance;
		return TRUE;
	case DLL_PROCESS_DETACH:
		return TRUE;
	}
	return TRUE;
}

GLOBALHOOKS_API void SetMouseHook ()
{
	if (mouseHook != NULL)
		return;
     
	lastMouseEventTime = ::GetTickCount ();
	mouseHook = ::SetWindowsHookEx (WH_MOUSE, (HOOKPROC)MouseHookProc, DllInstance,	0);
}

GLOBALHOOKS_API void ResetMouseHook ()
{
	::UnhookWindowsHookEx (mouseHook);
}

static LRESULT CALLBACK MouseHookProc (UINT code, WPARAM wParam, LPARAM lParam)
{
	// If nCode is less than zero, the hook procedure must pass the message to
	// the CallNextHookEx function without further processing and 
	// should return the value returned by CallNextHookEx.

	// If nCode is greater than or equal to zero and
	// if the hook procedure processed the message, 
	// it may return a nonzero value to prevent the system 
	// from passing the message to the target window procedure.

	if (code >= 0)
	{
		lastMouseEventTime = ::GetTickCount ();
	}
	return ::CallNextHookEx (mouseHook, code, wParam, lParam);
}

GLOBALHOOKS_API void SetKeyboardHook ()
{
	if (keyboardHook != NULL)
		return;
     
	lastKeyboardEventTime = ::GetTickCount ();
	keyboardHook = ::SetWindowsHookEx (WH_KEYBOARD, (HOOKPROC)KeyboardHookProc, DllInstance, 0);
}

GLOBALHOOKS_API void ResetKeyboardHook ()
{
	::UnhookWindowsHookEx (keyboardHook);
}

static LRESULT CALLBACK KeyboardHookProc (UINT code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0)
	{
		lastKeyboardEventTime = ::GetTickCount ();
	}
	return ::CallNextHookEx (keyboardHook, code, wParam, lParam);
}

bool GLOBALHOOKS_API HasBeenBusy (unsigned int withinPastNumberOfSeconds)
{
	unsigned int now = ::GetTickCount ();
	unsigned int inactivityPeriod = 1000 * withinPastNumberOfSeconds;

	return  (now - lastMouseEventTime)    < inactivityPeriod ||
		    (now - lastKeyboardEventTime) < inactivityPeriod;
}
