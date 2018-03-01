//-----------------------------------------------------
//  (c) Reliable Software 2001 -- 2003
//-----------------------------------------------------

#include "precompiled.h"
#include "Visitor.h"
#include "ProgressMeter.h"

#include <File/Dir.h>
#include <File/Path.h>

Traversal::Traversal (FilePath const & path, Visitor & visitor, ProgressMeter & progressMeter)
    : _visitor (visitor),
	  _progressMeter (progressMeter)
{
    TraverseTree (path, true);
}

void Traversal::TraverseTree (FilePath const & path, bool isRoot)
{
    FilePath curPath (path);
    for (FileSeq seq (curPath.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
    {
        char const * path = curPath.GetFilePath (seq.GetName ());
		_progressMeter.SetCaption (path);

        if (_visitor.Visit (path))
        {
            curPath.DirDown (seq.GetName ());
            TraverseTree (curPath, false);
            curPath.DirUp ();
			if (isRoot)
				_progressMeter.StepIt ();
        }
    }
}

