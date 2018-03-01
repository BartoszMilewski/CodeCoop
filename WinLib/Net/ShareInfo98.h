#if !defined (SHAREINFO_98_H)
#define SHAREINFO_98_H
//------------------------------
// (c) Reliable Software 2000-04
//------------------------------
#include "SharedObject.h"
#include <StringOp.h>
#include <svrapi.h>

namespace Net
{
	class ShareInfo50: public share_info_50
	{
	public:
		ShareInfo50 (SharedObject const & obj)
			: _name (obj.GetNetName ()), 
			  _path (obj.GetPath ()), 
			  _comment (obj.GetComment ())
		{
			Assert (_name.length () <= LM20_NNLEN);
			memset (this, 0, sizeof (share_info_50));
			
			// name must be all caps
			std::transform (_name.begin (), _name.end (),
				_name.begin (), 
				::ToUpper);

			// copy netname
			std::copy (_name.begin (), _name.end (), shi50_netname);

			// set other members
			shi50_type = (unsigned char) obj.GetType ();
			shi50_flags = SHI50F_FULL | SHI50F_PERSIST;	
			shi50_remark = &_comment [0];
			
			// drive letter must be capital
			std::transform (_path.begin (), _path.end (),
				_path.begin (), 
				::ToUpper);
			shi50_path = &_path [0];
			//shi50_rw_password
			//shi50_ro_password
		}

	private:
		std::string	_name;
		std::string	_path;
		std::string _comment;
	};
}

#endif
