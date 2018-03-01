#if !defined (OPENSAVEFILEDATA_H)
#define OPENSAVEFILEDATA_H
//---------------------------
// (c) Reliable Software 2010
//---------------------------
#include "CopyRequest.h"
#include <Bit.h>

class FileRequest
{
public:
	FileRequest (std::string const & typeValueName,
				 std::string const & ftpSiteValueName,
				 std::string const & folderValueName);

	void ReadNamedValues (NamedValues const & input);

	bool IsStoreOnMyComputer () const { return _copyRequest.IsMyComputer (); }
	bool IsStoreOnLAN () const { return _copyRequest.IsLAN (); }
	bool IsStoreOnInternet () const { return _copyRequest.IsInternet (); }
	bool IsOverwriteExisting () const { return _copyRequest.IsOverwriteExisting (); }
	bool IsAnonymousLogin () const { return _copyRequest.IsAnonymousLogin (); }
	virtual bool IsReadRequest () const = 0;

	std::string const & GetFileName () const { return _fileName; }
	std::string const & GetPath () const { return _copyRequest.GetLocalFolder (); }
	Ftp::SmartLogin & GetFtpLogin () { return _copyRequest.GetFtpLogin (); }

	void SetQuiet (bool flag) { _quiet = flag; }
	void SetMyComputer () { _copyRequest.SetMyComputer (); }
	void SetLAN () { _copyRequest.SetLAN (); }
	void SetInternet () { _copyRequest.SetInternet (); }
	void SetOverwriteExisting (bool bit) { _copyRequest.SetOverwriteExisting (bit); }

	void SetPath (std::string const & path) { _copyRequest.SetLocalFolder (path); }
	void SetFileName (std::string const & fileName) { _fileName = fileName; }

	bool IsValid ();
	std::string const & GetFileDescription () const { return _fileDescription; }
	void DisplayErrors (Win::Dow::Handle owner);

	void RememberUserPrefs () const;
	virtual bool OnApply () throw () { return true; }

private:
	enum Errors
	{
		NoFileName,
		NoTargetSelected,
		TargetExistsAndNoOverwrite,
		ExceptionThrown,
		FileDoesntExist
	};

private:
	// Registry value names for dialog preferences
	std::string		_typeValueName;
	std::string		_ftpSiteValueName;
	std::string		_folderValueName;

protected:
	std::string		_fileDescription;
	FileCopyRequest	_copyRequest;
	std::string		_fileName;
	bool			_quiet;

	BitSet<Errors>	_errors;
	std::string		_exceptionMsg;
};

class SaveFileRequest : public FileRequest
{
public:
	SaveFileRequest (std::string const & dialogCaption,
					 std::string const & fileNameFrameCaption,
					 std::string const & storeFrameCaption,
					 std::string const & typeValueName,
					 std::string const & ftpSiteValueName,
					 std::string const & folderValueName)
		: FileRequest (typeValueName, ftpSiteValueName, folderValueName),
		  _dialogCaption (dialogCaption),
		  _fileNameFrameCaption (fileNameFrameCaption),
		  _storeFrameCaption (storeFrameCaption)
	{}

	bool IsReadRequest () const { return false; }
	bool HasUserNote () const { return !_userNote.empty (); }

	std::string const & GetDialogCaption () const { return _dialogCaption; }
	std::string const & GetFileNameFrameCaption () const { return _fileNameFrameCaption; }
	std::string const & GetStoreFrameCaption () const { return _storeFrameCaption; }
	std::string const & GetUserNote () const { return _userNote; }

protected:
	std::string		_userNote;

private:
	std::string		_dialogCaption;
	std::string		_fileNameFrameCaption;
	std::string		_storeFrameCaption;
};

class OpenFileRequest : public FileRequest
{
public:
	OpenFileRequest (std::string const & dialogCaption,
					 std::string const & frameCaption,
					 std::string const & browseCaption,
					 std::string const & typeValueName,
					 std::string const & ftpSiteValueName,
					 std::string const & folderValueName)
		: FileRequest (typeValueName, ftpSiteValueName, folderValueName),
		  _dialogCaption (dialogCaption),
		  _frameCaption (frameCaption),
		  _browseCaption (browseCaption)
	{}

	bool IsReadRequest () const { return true; }

	std::string const & GetDialogCaption () const { return _dialogCaption; }
	std::string const & GetFrameCaption () const { return _frameCaption; }
	virtual char const * GetFileFilter () const { return ""; }
	std::string const & GetBrowseCaption () const { return _browseCaption; }

private:
	std::string	_dialogCaption;
	std::string	_frameCaption;
	std::string	_browseCaption;
};


#endif
