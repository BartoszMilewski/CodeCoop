#if !defined (SCCPROXY_H)
#define SCCPROXY_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2008
//------------------------------------

#include "SccOptions.h"

#include <windows.h>

namespace CodeCoop
{
	typedef void (*FunPtr) (); // generic function pointer
	typedef long (*TextOutProc) (char const * msg, unsigned long msgType);

	class Dll
	{
	public:
		Dll ();
		~Dll ();

		template <class T>
		void GetFunction (std::string const & funcName, T & funPointer) const
		{
			funPointer = reinterpret_cast<T> (GetFunction (funcName));
		}

	private:
		FunPtr GetFunction (std::string const & funcName) const;

		HINSTANCE _hDll;
	};

	class Proxy
	{
	public:
		Proxy (TextOutProc textOutCallback = 0);
		~Proxy ();

		bool StartCodeCoop ();
		bool CheckOut (int fileCount, char const **paths, SccOptions cmdOptions);
		bool UncheckOut (int fileCount, char const **paths, SccOptions cmdOptions);
		bool CheckIn (int fileCount, char const **paths, char const * comment, SccOptions cmdOptions);
		bool AddFile (int fileCount, char const **paths, std::string const & comment, SccOptions cmdOptions);
		bool RemoveFile (int fileCount, char const **paths, SccOptions cmdOptions);
		bool Status (int fileCount, char const **paths, long * status);
		bool IsControlledFile (char const * path, bool & isControlled, bool & isCheckedout);

	protected:
		Dll			_dll;
		void *		_context;	// Source code control provider execution context
	};
}

#endif
