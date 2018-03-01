#if !defined (USERDROPPREFERENCES_H)
#define USERDROPPREFERENCES_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "FileCtrlState.h"

class QuestionData
{
public:
	QuestionData () {}

	void Init (char const * projName,
			   char const * projFolder,
			   DropInfo const & fileInfo,
			   bool isMultiFileDrop);

	char const * GetProjName () const { return _projName; }
	char const * GetProjFolder () const { return _projFolder; }
	DropInfo const * GetDropInfo () const { return _fileInfo; }

	void SetYesToAll () { _yesToAll = true; }
	void SetNoToAll () { _noToAll = true; }
	void SetNo () { _no = true; }

	bool IsYesToAll () const { return _yesToAll; }
	bool IsNoToAll () const { return _noToAll; }
	bool IsNo () const { return _no; }
	bool IsMultiFileDrop () const { return _isMultiFileDrop; }

private:
	char const *		_projName;
	char const *		_projFolder;
	DropInfo const *_fileInfo;
	bool				_isMultiFileDrop;
	bool				_yesToAll;
	bool				_noToAll;
	bool				_no;
};

class DropPreferences
{
public:
	DropPreferences (Win::Dow::Handle hwnd,
					 std::string const & projectName,
					 bool canMakeChanges,
					 bool allControlledFileOverride,
					 bool allControlledFolderOverride)
		: _hwnd (hwnd),
		  _projectName (projectName),
		  _isMultiFileDrop (false)
	{
		if (canMakeChanges)
		{
			if (allControlledFileOverride)
				_option.set (OverwriteAllControlledFiles);
			if (allControlledFolderOverride)
				_option.set (OverwriteAllControlledFolders);
		}
		else
		{
			_option.set (NeverOverwriteControlled);
			_option.set (NeverAddToProject);
		}
	}

	void SetControlledState (bool notControlledSeen, bool controlledSeen);
	void SetMultiFileDrop (bool flag) { _isMultiFileDrop = flag; }

	bool AskCanOverwrite (char const * projFolder, DropInfo const & fileInfo);
	bool AskAddToProject (DropInfo const & fileInfo);

	bool CanOverwrite (DropInfo const & fileInfo);
	bool CanAddToProject ();

	bool GetAllControlledFilesOverride () const { return _option.test (OverwriteAllControlledFiles); }
	bool GetAllControlledFoldersOverride () const { return _option.test (OverwriteAllControlledFolders); }

	bool KeepAsking () const;

private:
	enum
	{
		OverwriteAllFiles,
		OverwriteAllFolders,
		OverwriteAllControlledFiles,
		OverwriteAllControlledFolders,
		NeverOverwrite,
		NeverOverwriteControlled,
		AddAllToProject,
		NeverAddToProject,
		LastQuestionAnswerWasYes,
		LastOption
	};

private:
	bool GetAllOverwritePreferences (bool & canOverwrite, DropInfo const & fileInfo) const;

private:
	Win::Dow::Handle		_hwnd;
	QuestionData			_dlgData;
	std::bitset<LastOption>	_option;
	std::string				_projectName;
	bool					_isMultiFileDrop;
};

#endif
