//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "precompiled.h"
#include "FileNames.h"
#include "SccProxy.h"
#include "OutputSink.h"

#include <File/Path.h>
#include <Ex/WinEx.h>
#include <Dbg/Assert.h>
#include <StringOp.h>

char const * FileSelectionName [FileMaxCount] = {
	"Current",
	"Before",
	"After",
};


FileNames::FileNames (XML::Node const * params)
	: _count (0),
	  _isReadOnly (true),
	  _isDiff (false),
	  _sccStatus (Unknown)
{
	if (params == 0)
		return;

	std::string const & operation = params->GetName ();
	if (operation == "diff")
		_isDiff = true;

	XML::Node::ConstChildIter it = params->FirstChild ();
	XML::Node::ConstChildIter last = params->LastChild ();
	while (it != last)
	{
		std::string const & role = (*it)->GetAttribValue ("role");
		if (role == "current")
			_files [FileCurrent] = (*it)->GetTransformAttribValue ("path");
		else if (role == "before")
			_files [FileBefore] = (*it)->GetTransformAttribValue ("path");
		else if (role == "after")
			_files [FileAfter] = (*it)->GetTransformAttribValue ("path");
		++it;
	}

	VerifyPaths ();

	if (HasPath (FileCurrent) && File::Exists (GetPath (FileCurrent)))
	{
		GetProjectFileTimeStamp ();
	}
}

void FileNames::Dump ()
{
#if !defined NDEBUG
	std::string msg;
	for (int i = 0; i < FileMaxCount; ++i)
	{
		FileSelection sel = static_cast<FileSelection> (i);
		if (HasPath (sel))
		{
			msg += "\n";
			msg += FileSelectionName [i];
			msg += ": ";
			msg += GetPath (sel);
		}
	}
	TheOutput.Display (msg.c_str ());
#endif
}

void FileNames::Clear ()
{
	_count = 0;
	_isReadOnly = true;
	_sccStatus = Unknown;
	_isDiff = false;
	_projFileTimeStamp.Clear ();
	_files.clear ();
}

void FileNames::TestBeforeFile ()
{
	std::string const & path = GetPath (FileBefore);
	if (path.empty () || !File::Exists (path))
	{
		_files [FileBefore] = "";
		_isDiff = false;
	}
}

void FileNames::SetProjectFile (std::string path)
{
	Assert (!path.empty ());
	NormalizePath (path);
	_files [FileCurrent] = path;
	VerifyPaths ();

	if (File::Exists (path))
	{
		GetProjectFileTimeStamp ();		
		_isReadOnly = File::IsReadOnly (GetPath (FileCurrent));
		GetSccStatus (true); // true force
	}
}

void FileNames::VerifyPaths ()
{
	_count = 0;
	for (FilePaths::const_iterator it = _files.begin (); it != _files.end (); ++it)
	{
		if (File::Exists (it->second))
			_count++;
	}
}

void FileNames::NormalizePath (std::string & path)
{
	PathSplitter splitter (path);
	if (splitter.HasOnlyFileName ())
	{
		CurrentFolder curFolder;
		PathMaker fullPath ("", curFolder.GetDir (), splitter.GetFileName (), splitter.GetExtension ());
		path.assign (fullPath);
	}
}

bool FileNames::GetProjectFileTimeStamp ()
{
	Assert (HasPath (FileCurrent));
	FileInfo fileInfo (GetPath (FileCurrent));
	FileTime curTimeStamp (fileInfo);
	if (!curTimeStamp.IsEqual (_projFileTimeStamp))
	{
		_projFileTimeStamp = curTimeStamp;
		return true;
	}
	return false;
}

bool FileNames::GetProjectFileReadOnlyState ()
{
	Assert (HasPath (FileCurrent));
	bool curReadOnlyState = File::IsReadOnly (GetPath (FileCurrent));
	if (_isReadOnly != curReadOnlyState)
	{
		_isReadOnly = curReadOnlyState;
		return true;
	}
	return false;
}

void FileNames::GetSccStatus (bool force)
{
	if (_sccStatus == Unknown || force)
	{
		if (force)
			_sccStatus = Unknown;
		try
		{
			if (HasPath (FileCurrent))
			{
				bool isControlled, isCheckedout;
				CodeCoop::Proxy proxy;
				if (proxy.IsControlledFile (GetPath (FileCurrent).c_str (), isControlled,  isCheckedout))
				{
					if (isControlled)
						_sccStatus = isCheckedout ? Checkedout : Controlled;
					else
						_sccStatus = NotControlled;
				}
			}
			else if (HasPath (FileAfter))
				_sccStatus = Controlled;
		}
		catch (...)
		{
			Win::ClearError ();
			_sccStatus = NotControlled;
		}
	}
}

void FileNames::GetProjectFileInfo (std::string & time, std::string & size) const
{
	Assert (HasPath (FileCurrent));
	FileInfo info (GetPath (FileCurrent));
    FileTime fileTime (info);
    PackedTimeStr modificationTime (fileTime);
	time.assign (modificationTime.c_str ());
	File::Size fileSize = info.GetSize ();
	size = FormatFileSize (fileSize.ToNative ().QuadPart);
}

std::string FileNames::GetProjectFileAttribute () const
{
	if (File::IsReadOnly (GetPath (FileCurrent)))
		return std::string ("read only");
	else return "read/write";
}

void FileNames::SetFileForComparing (std::string path)
{
	Assert (!path.empty ());
	NormalizePath (path);
	_files [FileBefore] = path;
	VerifyPaths ();
	_isDiff = (HasPath (FileCurrent) || HasPath (FileAfter));
}
