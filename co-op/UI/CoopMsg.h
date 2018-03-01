#if !defined (COOPMSG_H)
#define COOPMSG_H
//----------------------------------
// (c) Reliable Software 2002 - 2006
// ---------------------------------

#include <Ctrl/Messages.h>

// user messages
unsigned const UM_REFRESH_BROWSEWINDOW	= 1; // lParam = TableBrowser *
unsigned const UM_REFRESH_VIEWTABS		= 2;
unsigned const UM_CLOSE_PAGE			= 3; // wParam = page id

#endif
