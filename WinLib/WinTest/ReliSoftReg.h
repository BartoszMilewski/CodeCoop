#if !defined (RELISOFTREG_H)
#define RELISOFTREG_H
//-----------------------------
//  (c) Reliable Software, 2004
//-----------------------------
#include <Sys/RegKey.h>

namespace Registry
{
	class ReliSoftUser
	{
	public:
		ReliSoftUser (char const * appName)
		   : _keyMain (_keyRoot, "Software"),
			 _keyReliSoft (_keyMain, "Reliable Software"),
			 _keyApp (_keyReliSoft, appName)
		{}

		RegKey::Handle & Key () { return _keyApp; }

	private:
		RegKey::CurrentUser	_keyRoot;
		RegKey::Existing	_keyMain;
		RegKey::New			_keyReliSoft;
		RegKey::New			_keyApp;
	};
}

#endif
