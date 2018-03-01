#if !defined (REPORTDLG_H)
#define REPORTDLG_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2008
//------------------------------------

#include "SelectFileDlg.h"
#include "Table.h"

#include <File/Path.h>

class ReportRequest
{
public:
	ReportRequest (int currentProjectId, Table::Id tableId);

	void SetTargetPath (std::string const & path) { _path.Change (path); }
	void SetTargetFile (std::string const & fileName) { _reportFileName = fileName; }
	void SetTableId (Table::Id id) { _tableId = id; }

	char const * GetTargetPath () const { return _path.GetDir (); }
	char const * GetTargetFile () const { return _reportFileName.c_str (); }
	char const * GetTargetFilePath () const { return _path.GetFilePath (_reportFileName); }
	Table::Id GetTableId () const { return _tableId; }

private:
	std::string	_reportFileName;
	FilePath	_path;
	Table::Id	_tableId;
};

class ReportCtrl : public SelectFileCtrl
{
public:
	ReportCtrl (ReportRequest & dlgData);

	bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();

	bool GetDataFrom (NamedValues const & source);

private:
	ReportRequest & _dlgData;
};

#endif
