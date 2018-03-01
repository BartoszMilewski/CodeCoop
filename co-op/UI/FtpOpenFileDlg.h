#if !defined (FTPOPENFILEDLG_H)
#define FTPOPENFILEDLG_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <File/VPath.h>
#include <Net/Ftp.h>
#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/ListView.h>
#include <Ctrl/Button.h>

class FtpFileOpenData
{
public:
	FtpFileOpenData (std::string const & caption,
					 std::string const & server,
					 std::string const & folder)
		: _caption (caption),
		  _server (server),
		  _annonymousLogin (false),
		  _folder(folder, File::Vpath::UseFwdSlash)
	{
	}

	void SetAnonymousLogin (bool flag) { _annonymousLogin = flag; }
	void SetUser (std::string const & user) { _user = user; }
	void SetPassword (std::string const & password) { _password = password; }
	void ClearFolder () { _folder.SetPath (""); }
	void SetFileName (std::string const fileName) { _fileName = fileName; }
	void DirUp () { _folder.DirUp (); }
	void DirDown (std::string const & folder) { _folder.DirDown (folder); }

	bool IsAnonymousLogin () const { return _annonymousLogin; }

	std::string const & GetCaption () const { return _caption; }
	std::string const & GetServer () const { return _server; }
	std::string const   GetFolder () const { return _folder.ToString (); }
	std::string const & GetFileName () const { return _fileName; }
	std::string const & GetUser () const { return _user; }
	std::string const & GetPassword () const { return _password; }

private:
	std::string	_caption;
	std::string	_server;
	File::Vpath	_folder;
	std::string	_fileName;
	std::string	_user;
	std::string	_password;
	bool		_annonymousLogin;
};

class FtpFileOpenCtrl;

class FileListHandler : public Notify::ListViewHandler
{
public:
	explicit FileListHandler (FtpFileOpenCtrl & ctrl);

	bool OnDblClick () throw ();
	bool OnItemChanged (Win::ListView::ItemState & state) throw ();

private:
	FtpFileOpenCtrl & _ctrl;
};

class FtpFileOpenCtrl : public Dialog::ControlHandler
{
	friend class FileListHandler;

public:
	FtpFileOpenCtrl (FtpFileOpenData & dlgData, Win::Dow::Handle topWin);

	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	enum
	{
		imageFolder,
		imageFile,
		imageLast
	};

private:
	void SetCurrentFolder ();
	void Select (int itemIdx);
	bool OnDoubleClick ();

private:
	static int const	_iconIds [imageLast];
	ImageList::AutoHandle	_fileImages;
	Win::EditReadOnly	_server;
	Win::EditReadOnly	_folder;
	Win::Edit			_fileName;
	Win::Button			_goUp;
	Win::ReportListing	_content;
	FileListHandler		_notifyHandler;

	Internet::Access	_access;
	Internet::AutoHandle _hInternet;
	Ftp::Session		_session;
	Ftp::AutoHandle		_hFtp;

	FtpFileOpenData &	_dlgData;
};

#endif
