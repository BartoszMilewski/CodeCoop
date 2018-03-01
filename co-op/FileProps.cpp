//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "FileProps.h"
#include "FileData.h"
#include "ProjectDb.h"
#include "CheckoutNotifications.h"

FileProps::FileProps (FileData const & fileData,
					  Project::Db const & projectDb)
	: _gid (fileData.GetGlobalId ()),
	  _fileName (fileData.GetName ()),
	  _isCheckoutNotificationOn (false)
{
	MemberState memberState = projectDb.GetMemberState (projectDb.GetMyId ());
	_isCheckoutNotificationOn = memberState.IsCheckoutNotification ();
	FileState fileState = fileData.GetState ();
	if (fileState.IsRelevantIn (Area::Original))
		AddCheckedOutBy (projectDb, projectDb.GetMyId ());
}

std::string FileProps::GetCaption () const
{
	std::string caption (_fileName);
	caption += " ( Id: ";
	caption += GlobalIdPack (_gid).ToString ();
	caption += " ) Properties";
	return caption;
}

void FileProps::AddCheckoutNotifications (Project::Db const & projectDb,
										  CheckOut::Db const & checkoutNotificationDb)
{
	for (CheckOut::Db::Sequencer seq (checkoutNotificationDb, _gid);
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		UserId userId = seq.GetUserId ();
		AddCheckedOutBy (projectDb, userId);
	}
}

void FileProps::AddCheckedOutBy (Project::Db const & projectDb, UserId userId)
{
	std::unique_ptr<MemberInfo> member = projectDb.RetrieveMemberInfo (userId);
	std::string info (member->Name ());
	info += " ( Id: ";
	info += ::ToHexString (member->Id ());
	info += " )";
	_checkedOutBy.push_back (info);
}


