#if !defined (MAPISTATUSTABLE_H)
#define MAPISTATUSTABLE_H
//---------------------------------------------
// (c) Reliable Software 2004
//---------------------------------------------

#include "MapiIface.h"

namespace Mapi
{
	class DefaultDirectory;

	class StatusTable
	{
	public:
		StatusTable (DefaultDirectory & defDir);

		std::string GetLoggedProfileName ();

	private:
		Interface<IMAPITable>	_table;
	};
}

#endif
