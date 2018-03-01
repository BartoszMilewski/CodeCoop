#if !defined (GLOBALHOOKS_H)
#define GLOBALHOOKS_H
// ---------------------------
// (c) Reliable Software, 2004
// ---------------------------

#ifdef GLOBALHOOKS_EXPORTS
#define GLOBALHOOKS_API __declspec(dllexport)
#else
#define GLOBALHOOKS_API __declspec(dllimport)
#endif

#if defined( __cplusplus )
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC GLOBALHOOKS_API void SetMouseHook ();
EXTERNC GLOBALHOOKS_API void ResetMouseHook ();
EXTERNC GLOBALHOOKS_API void SetKeyboardHook ();
EXTERNC GLOBALHOOKS_API void ResetKeyboardHook ();
EXTERNC GLOBALHOOKS_API bool HasBeenBusy (unsigned int withinPastNumberOfSeconds);

#endif
