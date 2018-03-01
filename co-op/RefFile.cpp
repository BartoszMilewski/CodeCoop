//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "RefFile.h"
#include "PathFind.h"
#include "FileList.h"
#include "Diff.h"
#include "Conflict.h"
#include "BinDiffer.h"
#include "DiffRecord.h"
#include "ScriptCommands.h"
#include "FileData.h"

#include <Ctrl/ProgressMeter.h>
#include <Ex/WinEx.h>

// File is checked out and we need to create a diff cmd

ReferenceFile::ReferenceFile (char const * refPath)
    : _refPath (refPath)
{}

bool ReferenceFile::Diff (char const * newVersionPath, FileData const & fileData)
{
    MemFileReadOnly newVerFile (newVersionPath);
	File::Size newFileSize = newVerFile.GetSize ();
	if (newFileSize.IsLarge ())
		throw Win::InternalException ("File size exceeds 4GB", newVersionPath);

	unsigned long newLen = newFileSize.Low ();
    char const * newBuf = newVerFile.GetBuf ();
	
    MemFileReadOnly oldVerFile (_refPath.c_str ());
	CheckSum csFile (oldVerFile);
	if (fileData.GetCheckSum () != csFile)
	{
		throw Win::Exception ("Checksum mismatch: Original file corrupted.\nRun Repair from the Project menu", newVersionPath);
	}
   
	File::Size refFileSize = oldVerFile.GetSize ();
	if (refFileSize.IsLarge ())
		throw Win::Exception ("Reference file size exceeds 4GB", 0, 0);
	unsigned long lenRef = refFileSize.Low ();
    char const * bufRef = oldVerFile.GetBuf ();

	bool isDifferentCont = (newLen != lenRef || memcmp (newBuf, bufRef, newLen) != 0);	
    if (isDifferentCont)
	{
    
		_newCheckSum = CheckSum (newVerFile);
		if (fileData.GetType ().IsBinary ()
			|| lenRef > Differ::MaxFileSize || newLen > Differ::MaxFileSize)
		{
			_recorder = std::unique_ptr<DiffCmd> (new BinDiffCmd (fileData));
			BinDiffer binDiff (bufRef, File::Size (lenRef, 0), newBuf, File::Size (newLen, 0));
			binDiff.Record (*_recorder);
		}
		else
		{
			_recorder = std::unique_ptr<DiffCmd> (new TextDiffCmd (fileData));
			StrictComparator comp;
			Progress::Meter dummy;
			Differ diff (bufRef, lenRef, newBuf, newLen, comp, dummy);
			diff.Record (*_recorder);
		}
		// Final verification
		CheckSum verifyCheckSum (newVerFile);
		if (verifyCheckSum != _newCheckSum)
			throw Win::InternalException ("The file has been modified"
				"by an external application during the check-in.\n"
				"Close the application and try again.", newVersionPath);

	}
	else // files are equal
	{
		_newCheckSum = fileData.GetCheckSum ();
		_recorder = std::unique_ptr<DiffCmd> (new TextDiffCmd (fileData));
	}

	_recorder->SetNewFileSize (File::Size (newLen, 0));
	_recorder->SetNewCheckSum (_newCheckSum);

	_recorder->SetOldFileSize (File::Size (lenRef, 0));
	return isDifferentCont;
}

