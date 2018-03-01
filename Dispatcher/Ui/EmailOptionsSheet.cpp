// ----------------------------------
// (c) Reliable Software, 2005 - 2008
// ----------------------------------

#include "precompiled.h"
#include "EmailOptionsSheet.h"
#include "EmailPropertiesSheet.h"
#include "EmailConfigData.h"
#include "EmailConfig.h"
#include "ScriptProcessorConfig.h"
#include "EmailMan.h"
#include "AppInfo.h"

// Revisit: get rid of this function
bool Email::RunOptionsSheet (std::string const & myEmail,
							 Email::Manager & emailMan,
							 Win::Dow::Handle win,
							 Win::MessagePrepro * msgPrepro)
{
	Assert (emailMan.IsInTransaction ());
	EmailPropertiesSheet emailProperties (win, emailMan, myEmail, msgPrepro);
	return emailProperties.Display ();
}
