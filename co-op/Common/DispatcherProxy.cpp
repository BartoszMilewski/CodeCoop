//------------------------------------
//  (c) Reliable Software, 2002 - 2007
//------------------------------------

#include "precompiled.h"
#include "DispatcherProxy.h"
#include "DispatcherIpc.h"
#include "Global.h"
#include "PathRegistry.h"
#include "DispatcherScript.h"
#include "TransportHeader.h"
#include "Serialize.h"
#include "DispatcherMsg.h"

#include <File/Path.h>
#include <Com/Shell.h>
#include <File/File.h>
#include <Ex/Winex.h>

DispatcherProxy::DispatcherProxy ()
	: Win::ProcessProxy (DispatcherClassName),
	  _msgProjChange (UM_COOP_PROJECT_CHANGE),
	  _msgJoinRequest (UM_COOP_JOIN_REQUEST),
	  _msgCoopUpdate (UM_COOP_UPDATE),
	  _msgConfigWizard (UM_CONFIG_WIZARD),
	  _msgNotification (UM_COOP_NOTIFICATION),
	  _msgChunkSize (UM_COOP_CHUNK_SIZE_CHANGE)
{
	Win::Dow::Handle dispatcherWin = GetWin ();
	if (dispatcherWin.IsNull ())
	{
		// Running Dispatcher was not found -- start new instance
		FilePath programPath (Registry::GetProgramPath ());
		char const * dispatcherPath = programPath.GetFilePath ("Dispatcher.exe");
		if (File::Exists (dispatcherPath))
		{
			Win::ChildProcess dispatcher (dispatcherPath);
			dispatcher.SetCurrentFolder (programPath.GetDir ());
			dispatcher.ShowNormal ();
			Win::ClearError ();
			if (dispatcher.Create (5000)) // give the Dispatcher 5 sec to launch
			{
				Find (DispatcherClassName);
				Win::Dow::Handle dispatcherWin = GetWin ();
				if (dispatcherWin.IsNull ())
				{
					throw Win::Exception ("Cannot communicate with Code Co-op Dispatcher.");
				}
			}
			else
			{
				throw Win::Exception ("Cannot execute Code Co-op Dispatcher.");
			}
		}
		else
		{
			Win::ClearError ();
			throw Win::Exception ("Cannot find Code Co-op Dispatcher.  Run Code Co-op installation again.", dispatcherPath);
		}
	}
}

void DispatcherProxy::Notify (TransportHeader const & txHdr, DispatcherScript const & script)
{
	Win::Dow::Handle dispatcherWin = GetWin ();
	if (!dispatcherWin.IsNull ())
	{
		long flags = 0;
		DispatcherIpcHeader ipcHeader (flags);
		CountingSerializer counter;
		ipcHeader.Save (counter);
		txHdr.Save (counter);
		script.Save (counter);
		Assert (counter.GetSize () != 0);
		Assert (!LargeInteger(counter.GetSize ()).IsLarge ());
		unsigned size = static_cast<unsigned>(counter.GetSize());
		ipcHeader.SetSize (size);

		std::vector<unsigned char> buf (size);
		MemorySerializer out (&buf [0], buf.size ());
		ipcHeader.Save (out);
		txHdr.Save (out);
		script.Save (out);
		_msgNotification.SetWParam (buf.size ());
		_msgNotification.SetLParam (&buf [0]);
		dispatcherWin.SendInterprocessPackage (_msgNotification);
	}
}
