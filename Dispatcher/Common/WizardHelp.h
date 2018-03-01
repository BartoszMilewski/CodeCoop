#if !defined (WIZARDHELP_H)
#define WIZARDHELP_H
// -----------------------------
// (c) Reliable Software, 1999-2002
// -----------------------------

#include "AppHelp.h"
#include "AppInfo.h"

inline void OpenHelp ()
{
	AppHelp::Display (AppHelp::DispatcherTopic, "help", TheAppInfo.GetWindow());
}

#endif
