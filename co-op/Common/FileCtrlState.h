#if !defined (FILECTRLSTATE_H)
#define FILECTRLSTATE_H
//-----------------------------------
//  (c) Reliable Software 2001 - 2006
//-----------------------------------

#include <GlobalId.h>

#include <File/Path.h>

class FileDropInfoCollector;
class DropPreferences;

class FileControlState
{
public:
	void SetTargetExists () { _bits.set (TargetExistsOnDisk); }
	void SetCheckedOut () { _bits.set (CheckedOut); }
	void SetDeleted () { _bits.set (Deleted); }
		
	bool TargetExists () const { return _bits.test (TargetExistsOnDisk); }
	bool IsCheckedOut () const { return _bits.test (CheckedOut); }
	bool IsDeleted () const { return _bits.test (Deleted); }

private:
	enum
	{
		TargetExistsOnDisk,
		CheckedOut,
		Deleted,
		LastState
	};

private:
	std::bitset<LastState>	_bits;
};

class DropInfo
{
public:
	DropInfo (std::string const & sourcePath, std::string const & targetPath, FileControlState state)
		: _sourcePath (sourcePath),
		  _targetPath (targetPath),
		  _targetState (state)
	{
		PathSplitter splitter (sourcePath);
		_fileName = splitter.GetFileName ();
		_fileName += splitter.GetExtension ();
	}

	virtual bool RecordDropInfo (FileDropInfoCollector & collector, DropPreferences & userPrefs) const = 0;

	FileControlState const & GetCtrlState () const { return _targetState; }
	std::string const & GetFileName () const { return _fileName; }
	std::string const & GetSourcePath () const { return _sourcePath; }
	std::string const & GetTargetPath () const { return _targetPath; }
	virtual bool IsControlled () const { return false; }
	virtual bool IsFolder () const { return false; }

protected:
	FileControlState	_targetState;
	std::string			_fileName;
	std::string			_targetPath;
	std::string			_sourcePath;
};

#endif
