#if !defined (SHAREINFO_NT_H)
#define SHAREINFO_NT_H
//---------------------------
// (c) Reliable Software 2000-04
//---------------------------

#define UNICODE
#define FORCE_UNICODE // for VC++ 7.0

#include "SharedObject.h"
#include <Sys/Security.h>
#include <lm.h>

namespace Net
{
	class ShareInfo2: public SHARE_INFO_2
	{
	public:
		ShareInfo2 (SharedObject const & obj)
		{
			// zero memory
			memset (this, 0, sizeof (SHARE_INFO_2));
			shi2_netname = obj.NetName ();		// share netname (Unicode)
			shi2_type = obj.GetType ();// share type (folder, printer, ...)
			shi2_remark = obj.Comment ();		// comment
			shi2_permissions = ACCESS_ALL;		// share permissions
			shi2_max_uses = -1;					// max number of clients
			//shi2_current_uses = 0;			// number of current connections to the resource
			shi2_path = obj.Path ();			// local path for the shared resource (Unicode)
			//shi2_passwd = 0;					// share password
		}
		static unsigned Level () { return 2; }
	};

	class ShareInfo502: public SHARE_INFO_502
	{
	public:
		ShareInfo502 (SharedObject const & obj)
		{
			memset (this, 0, sizeof (SHARE_INFO_502));
			shi502_netname = obj.NetName ();		// share netname (Unicode)
			shi502_type = obj.GetType ();			// share type (folder, printer, ...)
			shi502_remark = obj.Comment ();		// comment
			shi502_permissions = ACCESS_ALL;		// share permissions
			shi502_max_uses = -1;					// max number of clients
			//shi2_current_uses = 0;				// number of current connections to the resource
			shi502_path = obj.Path ();				// local path for the shared resource (Unicode)
			//shi2_passwd = 0;						// share password
			//shi502_reserved = 0;
			//shi502_security_descriptor = 0;
		}
		void SetSecurity (Security::Descriptor const & security)
		{
			shi502_security_descriptor = security.Get ();
		}
		static unsigned Level () { return 502; }
	};
}	

#endif
