//------------------------------------
//  (c) Reliable Software, 2007 - 2009
//------------------------------------

#include "precompiled.h"
#include "CheckoutNotifications.h"

void CheckOut::FileUsers::AddUserId (GlobalId userId)
{
	if (std::find (_userIds.begin (), _userIds.end (), userId) == _userIds.end ())
		_userIds.push_back (userId);
}

void CheckOut::FileUsers::RemoveUserId (GlobalId userId)
{
	Iterator it = std::find (_userIds.begin (), _userIds.end (), userId);
	if (it != _userIds.end ())
		_userIds.erase (it);
}

void CheckOut::FileUsers::Serialize (Serializer & out) const
{
	out.PutLong (_fileId);
	_userIds.Serialize (out);
}

void CheckOut::FileUsers::Deserialize (Deserializer & in, int version)
{
	_fileId = in.GetLong ();
	_userIds.Deserialize (in, version);
}

void CheckOut::Db::XUpdate (GidList const & checkOutFiles, UserId senderId)
{
	// Make a local copy of file list to modify it
	GidList incomingCheckoutFiles = checkOutFiles;

	// Removes from incomingCheckoutFiles the files we have notes about
	XUpdateCheckoutNotes (incomingCheckoutFiles, senderId);

	// Remember fresh checkout notification from sender
	for (GidList::const_iterator iter = incomingCheckoutFiles.begin (); 
		iter != incomingCheckoutFiles.end (); ++iter)
	{
		GlobalId fileGid = *iter;
		std::unique_ptr<CheckOut::FileUsers> newNote (new CheckOut::FileUsers (fileGid, senderId));
		_notes.XAppend (std::move(newNote));
	}

	// Remove empty checkout notes from the database
	for (unsigned i = 0; i < _notes.XCount (); ++i)
	{
		CheckOut::FileUsers const * note = _notes.XGet (i);
		if (note != 0 && note->IsEmpty ())
			_notes.XMarkDeleted (i);
	}

	_noteIndex.clear ();
}

void CheckOut::Db::XPrepareForBranch ()
{
	// Remove checkout notes from the database
	for (unsigned i = 0; i < _notes.XCount (); ++i)
	{
		_notes.XMarkDeleted (i);
	}

	_noteIndex.clear ();
}

// Removes from incomingCheckoutFiles the files we already have notes about
void CheckOut::Db::XUpdateCheckoutNotes (GidList & incomingCheckoutFiles, UserId senderId)
{
	GidSet	incomingCheckoutSet (incomingCheckoutFiles.begin (), incomingCheckoutFiles.end ());

	for (iterator it = _notes.xbegin (); it != _notes.xend (); ++it)
	{
		CheckOut::FileUsers * note = *it;
		if (note == 0)
			continue;

		if (incomingCheckoutSet.find (note->GetFileId ()) == incomingCheckoutSet.end ())
		{
			// This file is not checked out by sender
			note->RemoveUserId (senderId);
		}
		else
		{
			// This file is checked out by sender
			// Add user id to its checkout user list.
			note->AddUserId (senderId); // Itempotent!
			if (!incomingCheckoutFiles.empty ())
			{
				// Remove this file from the reported list
				GidList::iterator iter = std::find (incomingCheckoutFiles.begin (),
													incomingCheckoutFiles.end (),
													note->GetFileId ());
				if (iter != incomingCheckoutFiles.end ())
					incomingCheckoutFiles.erase (iter);
			}
		}
	}
}

bool CheckOut::Db::IsCheckedOut (GlobalId gid) const
{
	if (_notes.Count () == 0)
		return false;

	RefreshIndex ();
	return _noteIndex.find (gid) != _noteIndex.end ();
}

bool CheckOut::Db::IsCheckedOutBy (UserId userId) const
{
	if (_notes.Count () == 0)
		return false;

	for (iterator noteIter = _notes.begin (); noteIter != _notes.end (); ++noteIter)
	{
		CheckOut::FileUsers const * note = *noteIter;
		std::vector<UserId> const & projectMembers = note->GetUserIds ();
		std::vector<UserId>::const_iterator iter = std::find (projectMembers.begin (),
															  projectMembers.end (),
															  userId);
		if (iter != projectMembers.end ())
			return true;
	}

	return false;
}

void CheckOut::Db::GetNotifyingMembers (GidList & members) const
{
	if (_notes.Count () == 0)
		return;

	GidSet checkedOutBy;
	for (iterator noteIter = _notes.begin (); noteIter != _notes.end (); ++noteIter)
	{
		CheckOut::FileUsers const * note = *noteIter;
		std::vector<UserId> const & projectMembers = note->GetUserIds ();
		checkedOutBy.insert (projectMembers.begin (), projectMembers.end ());
	}
	std::copy (checkedOutBy.begin (), checkedOutBy.end (), std::back_inserter (members));
}

CheckOut::Db::Sequencer::Sequencer (CheckOut::Db const & db, GlobalId gid)
	: _atEnd (true)
{
	db.RefreshIndex ();
	std::map<GlobalId, unsigned>::const_iterator iter = db._noteIndex.find (gid);
	if (iter != db._noteIndex.end ())
	{
		CheckOut::FileUsers const * note = db._notes.Get (iter->second);
		Assert (note != 0);
		_cur = note->GetUserIds ().begin ();
		_end = note->GetUserIds ().end ();
		_atEnd = (_cur == _end);
	}
}

void CheckOut::Db::Clear () throw ()
{
	TransactableContainer::Clear ();
}

void CheckOut::Db::Serialize (Serializer& out) const
{
	_notes.Serialize (out);
}

void CheckOut::Db::Deserialize (Deserializer& in, int version)
{
	CheckVersion (version, VersionNo ());
	_notes.Deserialize (in, version);
}

CheckOut::List::List (GidList const & fileIds)
	: _fileIds (fileIds),
	  _senderId (gidInvalid)
{
}

void CheckOut::Db::RefreshIndex () const
{
	if (_noteIndex.size () != 0)
		return;

	for (unsigned i = 0; i < _notes.Count (); ++i)
	{
		CheckOut::FileUsers const * note = _notes.Get (i);
		_noteIndex.insert (std::make_pair (note->GetFileId (), i));
	}
}

void CheckOut::List::Serialize (Serializer & out) const
{
	_fileIds.Serialize (out);
}

void CheckOut::List::Deserialize (Deserializer & in, int version)
{
	CheckVersion (version, VersionNo ());
	_fileIds.Deserialize (in, version);
}
