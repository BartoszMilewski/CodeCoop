//-------------------------------------
//  (c) Reliable Software, 1999 -- 2003
//-------------------------------------

#include "precompiled.h"
#include "AdminElection.h"
#include "ProjectDb.h"

AdminElection::AdminElection (Project::Db const & projectDb)
	: _curAdmin (0),
	  _newAdmin (0),
      _memberData(projectDb.RetrieveMemberList())
{
	std::vector<MemberInfo>::iterator iter = std::find_if (_memberData.begin (), _memberData.end (),
        [](MemberInfo const & member) {
            return member.IsAdmin();
    });
	if (iter != _memberData.end ())
	{
		_curAdmin = &*iter;
		// Change current project admin status to observer
		// and see if New one can be elected
		iter->MakeObserver ();
	}
	iter = ElectAdmin (_memberData.begin (), _memberData.end ());
	if (iter != _memberData.end ())
	{
		// New admin can be elected
		_newAdmin = &*iter;
	}
}
