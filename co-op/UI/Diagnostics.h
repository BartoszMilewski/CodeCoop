#if !defined (DIAGNOSTICS_H)
#define DIAGNOSTICS_H
//----------------------------------
// (c) Reliable Software 2005 - 2008
//----------------------------------

#include "SelectFileDlg.h"

class DiagnosticsRequest
{
public:
	DiagnosticsRequest (int currentProjectId);

	void SetVersion (bool bit)		{ _options.set (Version, bit); }
	void SetCatalog (bool bit)		{ _options.set (Catalog, bit); }
	void SetMembership (bool bit)	{ _options.set (Membership, bit); }
	void SetHistory (bool bit)		{ _options.set (History, bit); }
	void SetOverwriteExisting (bool bit) { _options.set (OverwriteExisting, bit); }
	void SetTargetPath (std::string const & path) { _path.Change (path); }
	void SetTargetFile (std::string const & fileName) { _diagFileName = fileName; }

	bool IsVersionDump () const		{ return _options.test (Version); }
	bool IsCatalogDump () const		{ return _options.test (Catalog); }
	bool IsMembershipDump () const	{ return _options.test (Membership); }
	bool IsHistoryDump () const		{ return _options.test (History); }
	bool IsOverwriteExisting () const { return _options.test (OverwriteExisting); }
	bool IsXMLDump () const;

	char const * GetTargetPath () const { return _path.GetDir (); }
	char const * GetTargetFile () const { return _diagFileName.c_str (); }
	char const * GetTargetFilePath () const { return _path.GetFilePath (_diagFileName); }

private:
	enum
	{
		Version,			// Dump windows and co-op versions
		Catalog,			// Dump global database (catalog)
		Membership,			// Dump project membership and its change history
		History,			// Dump file change history
		OverwriteExisting	// Overwrite existing diagnostics file
	};

private:
	std::string												_diagFileName;
	FilePath												_path;
	std::bitset<std::numeric_limits<unsigned long>::digits>	_options;
};

class DiagnosticsCtrl : public SelectFileCtrl
{
public:
	DiagnosticsCtrl (DiagnosticsRequest & dlgData);

	bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();

	bool GetDataFrom (NamedValues const & source);

private:
	DiagnosticsRequest & _dlgData;
};

#endif
