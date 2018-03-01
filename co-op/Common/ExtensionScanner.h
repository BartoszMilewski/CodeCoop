#if !defined (EXTENSIONSCANNER_H)
#define EXTENSIONSCANNER_H
//------------------------------------
//  (c) Reliable Software, 2002 - 2007
//------------------------------------

#include "Visitor.h"

#include <File/Path.h>
#include <StringOp.h>

#include <string>

namespace Progress { class Meter; }

class ExtensionScanner : public Visitor
{
public:
	ExtensionScanner (std::string const & root,
					  Progress::Meter & progressMeter);

	bool IsEmpty () const { return _extensions.empty (); }
	NocaseSet const & GetExtensions () { return _extensions; }
	bool Visit (char const * path);
	void CancelVisit () { }

private:
	FilePath	_root;
	NocaseSet	_extensions;
};

#endif
