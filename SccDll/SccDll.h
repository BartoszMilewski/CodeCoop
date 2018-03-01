//----------------------------------
// (c) Reliable Software 1999 - 2007
//----------------------------------

#include <Win/Instance.h>
#include <Sys/Synchro.h>
class Catalog;

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SCCDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SCCDLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef SCCDLL_EXPORTS
#define SCCDLL_API __declspec(dllexport)
#else
#define SCCDLL_API __declspec(dllimport)
#endif

class CatSingleton
{
public:
	CatSingleton () 
		: _critSection (true) // delay initialization
	{}

	void Init ()
	{
		_critSection.Init ();
	}
	Catalog & GetCatalog ();
private:
	Win::CritSection		_critSection;
	std::unique_ptr<Catalog>	_cat;
};

extern CatSingleton TheCatSingleton;
extern Win::Instance DllHInstance;
