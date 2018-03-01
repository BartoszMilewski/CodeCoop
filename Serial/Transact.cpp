//---------------------------------------------
// (c) Reliable Software 1997, 98, 99, 2000, 01
//---------------------------------------------

#include "precompiled.h"
#include "Transact.h"
#include "SysPath.h"
#include <Dbg/Assert.h>
#include <Dbg/Memory.h>
#include <Ex/WinEx.h>

// Transaction file names

char const dataFileOneName []  = "data1.bin";
char const dataFileTwoName []  = "data2.bin";

SwitchFile::SwitchFile ()
	: _dataState (badDataState),
      _phase (phaseUndo)
{}

// Create and initialize a New switch file
SwitchFile::SwitchFile (SysPathFinder const & pathFinder)
	: _dataState (dataFileOneValid),
      _phase (phaseUndo)
{
    File::Open (pathFinder.GetSwitchPath (), File::OpenAlwaysMode ());
    FlushState ();
}

void SwitchFile::Open (const char * fName)
{
    BYTE    b = 0;
    unsigned long   nBytes = 1;

    File::Open (fName, File::OpenAlwaysMode ());
    // Read status
	if (!SimpleRead (&b, nBytes) || nBytes != 1)
        throw Win::Exception ("Database error: Cannot read switch file", fName);
    if (b != dataFileOneValid && b != dataFileTwoValid)
		throw Win::Exception ("Database error: Switch file corrupt");
    _dataState = static_cast<DataState> (b);
	// Read phase
	nBytes = 1;
	if (!SimpleRead (&b, nBytes) || nBytes != 1)
	{
		b = phaseUndo;
	}
	if (b != phaseRedo && b != phaseUndo)
		throw Win::Exception ("Database error: Switch file corrupt");

	_phase = static_cast<TransPhase> (b);

    if (!Rewind ())
        throw Win::Exception ("Database error: Cannot rewind switch file");
}

void SwitchFile::FlushState ()
{
    BYTE b [3];
	b [0] = static_cast<BYTE> (_dataState);
    b [1] = static_cast<BYTE> (_phase);

    unsigned long nBytes = 3;
    if (!SimpleWrite (&b, nBytes) || nBytes != 3)
        throw Win::Exception ("Database error: Transaction commit failure - Cannot write to switch file");

    if (!Flush ())
        throw Win::Exception ("Database error: Transaction commit failure - Cannot flush switch file");
    if (!Rewind ())
        throw Win::Exception ("Database error: Cannot rewind switch file");
}

void SwitchFile::FlushStateOrDie ()
{
    BYTE b [3];
	b [0] = static_cast<BYTE> (_dataState);
    b [1] = static_cast<BYTE> (_phase);

    unsigned long nBytes = 3;
    if (!SimpleWrite (&b, nBytes) || nBytes != 3)
        throw Win::ExitException ("Database error: Transaction commit failure - Cannot write to switch file");

    if (!Flush ())
        throw Win::ExitException ("Database error: Transaction commit failure - Cannot flush switch file");
    if (!Rewind ())
        throw Win::ExitException ("Database error: Cannot rewind switch file");
}

ClearTransaction::ClearTransaction (Transactable& xable)
: _xable (xable), _commit (false)
{
	_xable.BeginClearTransaction ();
}

ClearTransaction::~ClearTransaction ()
{
    if (!_commit)
	{
		Dbg::NoMemoryAllocations noMemAllocs;
		noMemAllocs;	//	prevent warning C4101 by referencing variable
        _xable.AbortTransaction ();
	}
}

void ClearTransaction::Commit () throw ()
{
	{
		Dbg::NoMemoryAllocations noMemAllocs;
		noMemAllocs;	//	prevent warning C4101 by referencing variable
	    _xable.CommitTransaction ();
	}
	_xable.PostCommitTransaction ();
	_commit = true;
}

ReadTransaction::ReadTransaction (Transactable& xable, SysPathFinder& pathFinder)
: ClearTransaction (xable)
{
    _switch.Open (pathFinder.GetSwitchPath ());

    if (_switch.IsDataOneValid ())
    {
        _data.Open (pathFinder.GetSysFilePath (dataFileOneName), File::OpenAlwaysMode ());
    }
    else
    {
        _data.Open (pathFinder.GetSysFilePath (dataFileTwoName), File::OpenAlwaysMode ());
    }
}

Transaction::Transaction (Transactable& xable, SysPathFinder& pathFinder)
	: _xable (xable), _commit (false), _delete (false)
{
    _switch.Open (pathFinder.GetSwitchPath ());

    if (_switch.IsDataOneValid ())
    {
        _data.Open (pathFinder.GetSysFilePath (dataFileOneName), File::OpenAlwaysMode ());
    }
    else
    {
        _data.Open (pathFinder.GetSysFilePath (dataFileTwoName), File::OpenAlwaysMode ());
    }


    if (_switch.IsDataOneValid ())
    {
        _backup.Open (pathFinder.GetSysFilePath (dataFileTwoName),
					  File::OpenAlwaysMode ());
    }
    else
    {
        _backup.Open (pathFinder.GetSysFilePath (dataFileOneName), 
					  File::OpenAlwaysMode ());
    }
    _xable.BeginTransaction ();
}

Transaction::~Transaction ()
{
    if (_delete)
    {
        // Transaction files have been deleted
        // There is nothing to do
    }
	else
	{
		if (_commit)
		{
			_data.Empty ();
		}
		else
		{
			Assert (_delete == false);
			_backup.Empty ();
		}
	}

    if (!_commit)
	{
		Dbg::NoMemoryAllocations noMemAllocs;
		noMemAllocs;	//	prevent warning C4101 by referencing variable
        _xable.AbortTransaction ();
	}
}

void Transaction::CommitPhaseOne ()
{
	if (_switch.MustRedo ())
		throw Win::Exception ("Database error: previous transaction was interrupted");
	// Prepare for redoing
	_switch.SetRedo ();
	Commit ();
}

void Transaction::CommitPhaseTwo ()
{
	// Go back to undo state
	_switch.SetUndo ();
    _switch.FlushStateOrDie ();
}

void Transaction::Commit ()
{
    if (_switch.IsDataOneValid ())
        _switch.MakeDataTwoValid ();
    else
        _switch.MakeDataOneValid ();

    _xable.Save (_backup);
    _backup.Flush ();
    _switch.FlushState ();
	_commit = true; // must change immediately after flushing _switch

	{
		Dbg::NoMemoryAllocations noMemAllocs;
		noMemAllocs;	//	prevent warning C4101 by referencing variable
		_xable.CommitTransaction ();
	}
	_xable.PostCommitTransaction ();
}

void Transaction::CommitAndDelete (SysPathFinder& pathFinder)
{
    // Commit transaction -- this leaves all data strucutres
    // in well defined state
    Commit ();

    // Delete transaction files
    _switch.Close();
    _data.Close ();
    _backup.Close ();

    char const *filePath = pathFinder.GetSwitchPath ();
    File::DeleteNoEx (filePath);

    filePath = pathFinder.GetSysFilePath (dataFileOneName);
    File::DeleteNoEx (filePath);

    filePath = pathFinder.GetSysFilePath (dataFileTwoName);
    File::DeleteNoEx (filePath);

    _delete = true;
}

// Staging cleanup is done when visiting a project
// If a staging transaction was interrupted we have to redo it.
// Otherwise, the staging area is cleared.

StagingCleanup::StagingCleanup (SysPathFinder & sysPathFinder)
{
    _switch.Open (sysPathFinder.GetSwitchPath ());
}

bool StagingCleanup::MustRedo ()
{
	return _switch.MustRedo ();
}

void StagingCleanup::Commit ()
{
	_switch.SetUndo ();
	_switch.FlushState ();
}
