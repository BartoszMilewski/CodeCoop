// Reliable Software (c) 2002
#include "precompiled.h"
#include "Session.h"
#include "Lister.h"
#include "IPCExchange.h"
#include "PathRegistry.h"
#include <File/File.h>
#include <Sys/Process.h>
#include <XML/XmlTree.h>

Session::~Session () 
{}

void Session::QueryDone ()
{
	_worker.reset ();
}

// Data::Sink
void Session::DataReady (Data::ChunkHolder data, bool isDone, int idSrc)
{
	if (isDone)
	{
		dbg << "Session done" << std::endl;
	}
	_sink.DataReady (data, isDone, idSrc);
}

void SessionDir::Refresh (bool restart)
{
	if (restart)
	{
		if (_path.IsDirStrEmpty ())
		{
			_worker.reset (new DriveLister (this, _id));
		}
		else
		{
			Data::ListQuery query (_path);
			_worker.reset (new Lister (query, this, _id));
		}
	}
	else
		_worker.reset ();
}

bool SessionDiff::IsDiff () const 
{ 
	return !_path1.IsDirStrEmpty () 
		&& !_path2.IsDirStrEmpty () 
		&& _dirDepth1 == _dirDepth2
		&& !IsNocaseEqual (_path1.GetDir (), _path2.GetDir ()); 
}

std::string SessionDiff::GetCaption () const
{
	std::string caption;
	if (IsDiff ())
	{
		caption = GetOldDir ();
		caption += " -> ";
		caption += GetNewDir ();
	}
	else if (_dirDepth1 > _dirDepth2)
	{
		caption = "<Only present in old>  ";
		caption += GetOldDir ();
	}
	else if (_dirDepth1 < _dirDepth2)
	{
		caption = "<Only present in new>  ";
		caption += GetNewDir ();
	}
	return caption;
}

void SessionDiff::Refresh (bool restart)
{
	if (restart)
	{
		//_worker.SetKillTimeout (5000);
		if (!_path1.IsDirStrEmpty () && !_path2.IsDirStrEmpty () 
			&& !IsNocaseEqual (_path1.GetDir (), _path2.GetDir()))
		{
			Data::CmpQuery query (_path1, _path2);
			_worker.reset (new Comparator (query, this, _id));
		}
		else if (!_path1.IsDirStrEmpty ())
		{
			_dirDepth2 = -1;
			Data::ListQuery query (_path1);
			_worker.reset (new Lister (query, this, _id));
		}
		else if (!_path2.IsDirStrEmpty ())
		{
			_dirDepth1 = -1;
			Data::ListQuery query (_path2);
			_worker.reset (new Lister (query, this, _id));
		}
		else
		{
			_dirDepth2 = -1;
			_worker.reset (new DriveLister (this, _id));
		}
	}
	else
		_worker.reset ();
}

void Session::CallDiffer (XML::Tree const & xmlArgs)
{
	// Assume Code Co-op is installed
	std::string differPath = Registry::GetProgramPath ();
	if (differPath.empty ())
		throw Win::Exception ("You have to install Code Co-op to view files");
	differPath += '\\';
	differPath += "Differ.exe";

	XmlBuf buf;
	std::ostream out (&buf); // use buf as streambuf
	xmlArgs.Write (out);
	if (out.fail ())
		throw Win::InternalException ("Differ argument string too long.");

	std::ostringstream cmdLine;
	cmdLine << " /xmlspec 0x" << std::hex << buf.GetHandle ();
	Win::ChildProcess process (cmdLine.str ().c_str (), true);	// Inherit parent's handles
	process.SetAppName (differPath);

	if (!process.Create ())
		throw Win::Exception ("Cannot start Differ.exe");
}

void SessionDir::OpenFile (std::string const & name, DiffState state)
{
	XML::Tree xmlTree;
	XML::Node * root = xmlTree.SetRoot ("edit");
	char const * filePath = _path.GetFilePath (name);
	XML::Node * fileNode = root->AddEmptyChild ("file");
	fileNode->AddAttribute ("path", filePath);
	fileNode->AddAttribute ("role", "current");
	CallDiffer (xmlTree);
}

void SessionDiff::OpenFile (std::string const & name, DiffState state)
{
	XML::Tree xmlTree;
	XML::Node * root = xmlTree.SetRoot ("edit");
	XML::Node * fileNode = root->AddEmptyChild ("file");
	if (state == stateNew || _dirDepth2 > _dirDepth1)
	{
		char const * filePath = _path2.GetFilePath (name);
		fileNode->AddAttribute ("role", "current");
		fileNode->AddAttribute ("path", filePath);
	}
	else if (state == stateDel || _dirDepth1 > _dirDepth2)
	{
		char const * filePath = _path1.GetFilePath (name);
		fileNode->AddAttribute ("role", "before");
		fileNode->AddAttribute ("path", filePath);
	}
	else
	{
		root->SetName ("diff");
		Assert (state == stateSame || state == stateDiff);
		Assert (_dirDepth1 == _dirDepth2);
		std::string path1 (_path1.GetFilePath (name));
		std::string path2 (_path2.GetFilePath (name));
		Assert (!File::IsFolder (path1.c_str ()));
		Assert (!File::IsFolder (path2.c_str ()));
		fileNode->AddAttribute ("role", "current");
		fileNode->AddAttribute ("path", path2);

		fileNode = root->AddEmptyChild ("file");
		fileNode->AddAttribute ("role", "before");
		fileNode->AddAttribute ("path", path1);
	}
	CallDiffer (xmlTree);
}

void SessionDir::DirDown (std::string const & name, DiffState state)
{
	_sink.DataClear ();
	_path.DirDown (name.c_str ());
	Data::ListQuery query (_path);
	_worker.reset (new Lister (query, this, _id));
}

void SessionDiff::DirDown (std::string const & name, DiffState state)
{
	_sink.DataClear ();
	if (state == stateNew || _dirDepth2 > _dirDepth1)
	{
		char const * path = _path2.GetFilePath (name);
		_path2.DirDown (name.c_str ());
		++_dirDepth2;
		Data::ListQuery query (_path2);
		_worker.reset (new Lister (query, this, _id));
	}
	else if (state == stateDel || _dirDepth1 > _dirDepth2)
	{
		char const * path = _path1.GetFilePath (name);
		_path1.DirDown (name.c_str ());
		++_dirDepth1;
		Data::ListQuery query (_path1);
		_worker.reset (new Lister (query, this, _id));
	}
	else if (state == stateSame)
	{
		Assert (_dirDepth1 == _dirDepth2);
		std::string path1 (_path1.GetFilePath (name));
		std::string path2 (_path2.GetFilePath (name));
		_path1.DirDown (name.c_str ());
		++_dirDepth1;
		_path2.DirDown (name.c_str ());
		++_dirDepth2;
		Data::CmpQuery query (_path1, _path2);
		_worker.reset (new Comparator (query, this, _id));
	}
}

void SessionDir::DirUp ()
{
	_sink.DataClear ();

	FilePathSeq folderSeq (_path.GetDir ());
	if (folderSeq.IsDriveOnly ())
		_path.Clear ();
	else
		_path.DirUp ();

	Win::Lock lock (_critSect);
	if (_path.IsDirStrEmpty ())
	{
		_worker.reset (new DriveLister (this, _id));
	}
	else
	{
		Data::ListQuery query (_path);
		_worker.reset (new Lister (query, this, _id));
	}
}

void SessionDiff::DirUp ()
{
	if (_dirDepth1 == 0 && _dirDepth2 == 0)
		return;

	_sink.DataClear ();
	if (_dirDepth1 > _dirDepth2)
	{
		_path1.DirUp ();
		--_dirDepth1;
	}
	else if (_dirDepth1 < _dirDepth2)
	{
		_path2.DirUp ();
		--_dirDepth2;
	}
	else
	{
		_path1.DirUp ();
		--_dirDepth1;
		_path2.DirUp ();
		--_dirDepth2;
	}

	Win::Lock lock (_critSect);
//	list.Initialize (_path1.GetDir (), _path2.GetDir (), list.GetColWidthPct ());
	if (_dirDepth1 > _dirDepth2)
	{
		Data::ListQuery query (_path1);
		_worker.reset (new Lister (query, this, _id));
	}
	else if (_dirDepth1 < _dirDepth2)
	{
		Data::ListQuery query (_path2);
		_worker.reset (new Lister (query, this, _id));
	}
	else
	{
		Data::CmpQuery query (_path1, _path2);
		_worker.reset (new Comparator (query, this, _id));
	}
}
