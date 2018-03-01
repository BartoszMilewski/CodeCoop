#if !defined (CMDDATA_H)
#define CMDDATA_H
//-------------------------------------------------
//  CmdData.h
//  (c) Reliable Software, 2000 -- 2002
//-------------------------------------------------

#include "FileClassifier.h"

class CmdData
{
public:
	CmdData (std::string const & name, void * context)
		: _name (name), 
		  _fileCount (0), 
		  _ideContext (reinterpret_cast<IDEContext *>(context)),
		  _classifier (0, 0, reinterpret_cast<IDEContext *>(context))
	{}
	CmdData (std::string const & name, long fileCount, char const **paths, void * context)
		: _name (name),
		  _classifier (fileCount, paths, reinterpret_cast<IDEContext *>(context)),
		  _fileCount (fileCount),
		  _ideContext (reinterpret_cast<IDEContext *>(context))
	{}

	std::string const & GetCmdName () const { return _name; }
	long FileCount () const { return _fileCount; }
	FileListClassifier::Iterator ProjectBegin () const { return _classifier.begin (); }
	FileListClassifier::Iterator ProjectEnd () const { return _classifier.end (); }
	IDEContext const * GetIdeContext () const { return _ideContext; }

private:
	std::string			_name;
	FileListClassifier	_classifier;
	long				_fileCount;
	IDEContext const *  _ideContext;
};

#endif
