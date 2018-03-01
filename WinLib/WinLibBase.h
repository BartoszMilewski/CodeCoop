#if !defined (WINLIBBASE_H)
#define WINLIBBASE_H

#define _WIN32_DCOM

// DBG_LOGGING: true/false
// if you want to turn off debug logging in a specific file,
// add these lines after #include "precompiled.h":
// #undef DBG_LOGGING
// #define DBG_LOGGING false
// Note: do not change the definition of this macro inside header files,
// because this also changes logging settings in files that include the headers.
// Change the value of the macro only in source files!

#define DBG_LOGGING true

// Tool tips Help: Several members of the NOTIFYICONDATA structure are only supported for
// Shell32.dll versions 5.0 and later. To enable these members, include the following in your header: 
#define _WIN32_IE 0x0501

// GetLastInputInfo
#define _WIN32_WINNT 0x0500

#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <shlobj.h>
#include <crtdbg.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <limits>
#include <functional>
#include <map>
#include <list>
#include <set>
#include <iterator>

#include <Ex/WinEx.h>
#include <Dbg/All.h>
#include <Win/Win.h>

#endif
