#if !defined (CMPDIRREG_H)
#define CMPDIRREG_H
//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include "ReliSoftReg.h"

namespace Registry
{
	class CmpDirUser: public ReliSoftUser
	{
	public:
		CmpDirUser (char const * keyName)
			: ReliSoftUser ("CmpDir"),
			_branch (ReliSoftUser::Key (), keyName)
		{}
		RegKey::Handle const & Key () const { return _branch; }
	private:
		RegKey::New	_branch;
	};

}
#endif
