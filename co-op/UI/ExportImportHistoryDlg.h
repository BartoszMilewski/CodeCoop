#if !defined (EXPORTIMPORTHISTORYDLG_H)
#define EXPORTIMPORTHISTORYDLG_H
//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include "OpenSaveFileDlg.h"

namespace History
{
	class ExportRequest : public SaveFileRequest
	{
	public:
		ExportRequest (std::string const & projectName);
	};

	class OpenRequest : public OpenFileRequest
	{
	public:
		OpenRequest ();

		char const * GetFileFilter () const { return "Code Co-op History File (*.his)\0*.his\0All Files (*.*)\0*.*"; }
	};
}
#endif
