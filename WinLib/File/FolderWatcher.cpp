//----------------------------------
//  (c) Reliable Software, 1997-2005
//----------------------------------
#include <WinLibBase.h>
#include "FolderWatcher.h"

#include <Win/Message.h>
#include <Ctrl/Messages.h>
#include <File/File.h>

void FWatcher::LokPostChange (std::string const & change)
{
	if (!_win.IsNull ())
	{
		_changes.insert (change);
		Win::UserMessage msg (UM_FOLDER_CHANGE);
		msg.SetLParam (this);
		_win.PostMsg (msg);
	}
}

bool FWatcher::RetrieveChange (std::string & change)
{
	//------------------------
	Win::Lock lock (_critSect);
	std::set<std::string>::iterator it = _changes.begin ();
	if (it == _changes.end ())
		return false;
	change = *it;
	_changes.erase (it);
	return true;
	//------------------------
}

void FolderWatcher::Run ()
{
    Assert (_folderEvent.IsValid ());
    HANDLE objects [2];
    objects [0] = _folderEvent.ToNative ();
    objects [1] = _event.ToNative ();
    for (;;)
    {
        // Wait for change notification
        DWORD waitStatus = ::WaitForMultipleObjects (2, objects, FALSE, INFINITE);
        if (waitStatus == WAIT_OBJECT_0)
        {
            if (IsDying ())
                return;
			//------------------------
			Win::Lock lock (_critSect);
			LokPostChange (_folderEvent.GetDir ());

			// Continue change notification
			if (!_folderEvent.ContinueNotification ())
			{
				// Continuation failed
				return;
			}
			//------------------------
        }
        else if (WAIT_OBJECT_0 + 1 == waitStatus)
        {
            // End of watch duty - _event was released
            return;
        }
        else
        {
            // Wait failed
			throw Win::Exception ("Failed wait for changes in folder", 
									_folderEvent.GetDir ().c_str ());
			return;
        }
    }
}

MultiFolderWatcher::MultiFolderWatcher (std::vector<std::string> const & folders, 
										Win::Dow::Handle win)
    : FWatcher (win),
	  _refresh (true)
{
	typedef std::vector<std::string>::const_iterator Iter;
	// Prepare the watch list
	for (Iter it = folders.begin (); it != folders.end (); ++it)
    {
		// non-recursive
		_folders.push_back (std::unique_ptr<WatchedFolder> (new WatchedFolder (*it, false)));
    }
}

MultiFolderWatcher::MultiFolderWatcher (Win::Dow::Handle win)
    : FWatcher (win),
	  _refresh (true)
{
}

void MultiFolderWatcher::AddFolder (std::string const & folder, bool recursive)
{
	//------------------------
	Win::Lock lock (_critSect);
	FolderIter it = std::find_if (_folders.begin (), _folders.end (), 
								  MultiFolderWatcher::IsEqualDir (folder));
	if (it == _folders.end ())
	{
		// Folder not on the watch list -- add it
		_folders.push_back (std::unique_ptr<WatchedFolder> (new WatchedFolder (folder, recursive)));
		_refresh = true;
		// Wakeup watcher thread, so it can refresh its event list
		FlushThread ();
	}
	else if ((*it)->IsMarkedStopWatching ())
	{
		(*it)->RestartWatching ();
	}
	//------------------------
}

void MultiFolderWatcher::AddFolders (std::vector<std::string> const & folders)
{
	//------------------------
	Win::Lock lock (_critSect);
	for (std::vector<std::string>::const_iterator newFolder = folders.begin ();
	     newFolder != folders.end ();
		 ++newFolder)
	{
		FolderIter it = std::find_if (_folders.begin (), _folders.end (), 
			MultiFolderWatcher::IsEqualDir (*newFolder));
		if (it == _folders.end ())
		{
			// Folder not on the watch list -- add it (non-recursive!)
			_folders.push_back (std::unique_ptr<WatchedFolder> (new WatchedFolder (*newFolder, false)));
			_refresh = true;
		}
	}
	if (_refresh)
	{
		// Wakeup watcher thread, so it can refresh its event list
		FlushThread ();
	}
	//------------------------
}

void MultiFolderWatcher::StopWatching (std::string const & folder) throw ()
{
	try
	{
		//------------------------
		Win::Lock lock (_critSect);
		FolderIter it = std::find_if (_folders.begin (), _folders.end (), 
									  MultiFolderWatcher::IsEqualDir (folder));
		if (it != _folders.end ())
		{
			(*it)->MarkStopWatching ();
			_refresh = true;
			// Wakeup watcher thread, so it can refresh its event list
			FlushThread ();
		}
		//------------------------
	}
	catch (...)
	{
		Win::ClearError ();
	}
}

void MultiFolderWatcher::Run ()
{
    std::vector<HANDLE> objects;			// events for watched folders
	std::vector<int>	objectToFolderIdx; // there are more folders than object
    for (;;)
    {
		{
			//------------------------
			Win::Lock lock (_critSect);
			if (_refresh)
			{
				// Remove stopped watchers from the watch list
				size_t i = 0;
				while (i < _folders.size ())
				{
					if (_folders [i]->IsMarkedStopWatching ())
					{
						_folders.erase (i);	// Removes item and moves up items after the removed one
					}
					else
						++i;
				}
				// Copy file change events for watched folders
				// and close change events for not watched folders
				objects.clear ();
				objectToFolderIdx.clear ();
				for (size_t i = 0; i < _folders.size (); ++i)
				{
					WatchedFolder * folder = _folders [i];
					if (objects.size () < MAXIMUM_WAIT_OBJECTS - 1)
					{
						objects.push_back (folder->GetEvent ());
						objectToFolderIdx.push_back (i);
					}
					else
					{
						folder->StopWatching ();
						// During next refresh, change event will be closed
					}
				}
				Assert (objects.size () == objectToFolderIdx.size ());
				objects.push_back (_event.ToNative ());
				Assert (objects.size () <= MAXIMUM_WAIT_OBJECTS);
				_refresh = false;
			}
			//------------------------
		}
        //----- Wait for change notification -------
        DWORD waitStatus = ::WaitForMultipleObjects (objects.size (), &(objects [0]), FALSE, INFINITE);
		if (IsDying ())
			return;

		if (waitStatus == WAIT_TIMEOUT)
			continue;
		if (waitStatus == WAIT_FAILED)
			throw Win::Exception ("Directory watcher failed");
        unsigned int eventIdx = waitStatus - WAIT_OBJECT_0;
		if (eventIdx >= objects.size ())
			throw Win::Exception ("Unexpected status in directory watcher");
		Assert (objects.size () == objectToFolderIdx.size () + 1);
        if (eventIdx < objectToFolderIdx.size ())
        {
            // Folder change notification
			//------------------------
			Win::Lock lock (_critSect);
			// Translate wait event index to folder index
			unsigned int idx = objectToFolderIdx [eventIdx];
			Assert (0 <= idx && idx < _folders.size ());
			WatchedFolder * folder = _folders [idx];
			if (!folder->IsMarkedStopWatching ())
			{
				LokPostChange (folder->GetDir ());
				// Continue change notification
				if (!folder->ContinueNotification ())
				{
					// Continuation failed -- stop watching folder
					folder->MarkStopWatching ();
					_refresh = true;	// Refresh the watch list
				}
			}
			else
			{
				// Refresh the watch list
				_refresh = true;
			}
			//------------------------
        }
    }
}
