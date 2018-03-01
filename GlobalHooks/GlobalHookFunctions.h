#if !defined (GLOBALHOOKFUNCTIONS_H)
#define GLOBALHOOKFUNCTIONS_H
// ---------------------------
// (c) Reliable Software, 2004
// ---------------------------

typedef void (*SetMouseHookPtr)     ();
typedef void (*ResetMouseHookPtr)   ();
typedef void (*SetKeyboardHookPtr)  ();
typedef void (*ResetKeyboardHookPtr)();
typedef bool (*HasBeenBusyPtr) (unsigned int withPastNumberOfSeconds);

#endif
