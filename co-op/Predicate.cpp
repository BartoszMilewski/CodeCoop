//----------------------------------
// (c) Reliable Software 1998 - 2006
//----------------------------------

#include "precompiled.h"
#include "Predicate.h"
#include "FileState.h"
#include "FileTypes.h"
#include "HistoryScriptState.h"
#include "ProjectState.h"
#include "MailboxScriptState.h"
#include "MergeStatus.h"
#include "StringOp.h"

std::function<bool(long, long)> IsStateNone = 
[](unsigned long state, unsigned long type) -> bool
{
    FileState fileState (state);
    return fileState.IsNone();
};

std::function<bool(long, long)> IsStateInProject =
[](unsigned long state, unsigned long type) -> bool
{
    FileState fileState (state);
	FileType fileType (type);
    return !fileState.IsNone() && !fileType.IsRoot ();
};

std::function<bool(long, long)> IsCheckedIn =
[](unsigned long state, unsigned long type) -> bool
{
    FileState fileState (state);
    return fileState.IsCheckedIn ();
};

std::function<bool(long, long)> IsCheckoutable =
[](unsigned long state, unsigned long type) -> bool
{
	FileState fileState (state);
	return fileState.IsPresentIn (Area::Project) && !fileState.IsRelevantIn (Area::Original);
};

std::function<bool(long, long)> IsFileCheckedIn =
[](unsigned long state, unsigned long type) -> bool
{
    FileState fileState (state);
	FileType fileType (type);
    return fileState.IsCheckedIn () && !fileType.IsFolder ();
};

std::function<bool(long, long)> IsFolder =
[](unsigned long state, unsigned long type) -> bool
{
	FileType fileType (type);
    return fileType.IsFolder () || fileType.IsRoot ();
};

std::function<bool(long, long)> IsFolderCheckedIn =
[](unsigned long state, unsigned long type) -> bool
{
    FileState fileState (state);
	FileType fileType (type);
    return fileState.IsCheckedIn () && fileType.IsFolder () || fileType.IsRoot ();
};

std::function<bool(long, long)> IsNewFolder =
[](unsigned long state, unsigned long type) -> bool
{
    FileState folderState (state);
	FileType itemType (type);
    return itemType.IsFolder () && folderState.IsNew ();
};

std::function<bool(long, long)> IsCheckedOutFolderButNotNew =
[](unsigned long state, unsigned long type) -> bool
{
    FileState folderState (state);
	FileType itemType (type);
    return itemType.IsFolder () && !folderState.IsCheckedIn () && !folderState.IsNew ();
};

std::function<bool(long, long)> IsFolderDeletedBySynch =
[](unsigned long state, unsigned long type) -> bool
{
    FileState folderState (state);
	FileType itemType (type);
    return itemType.IsFolder () && folderState.IsRelevantIn (Area::Synch) && !folderState.IsPresentIn (Area::Synch);
};

std::function<bool(long, long)> IsNotPresentFolder =
[](unsigned long state, unsigned long type) -> bool
{
    FileState folderState (state);
	FileType itemType (type);
    return itemType.IsFolder () && !folderState.IsPresentIn (Area::Project);
};

std::function<bool(long, long)> IsCheckedOut =
[](unsigned long state, unsigned long type) -> bool
{
    FileState fileState (state);
	return fileState.IsRelevantIn (Area::Original);
};

std::function<bool(long, long)> IsFile =
[](unsigned long state, unsigned long type) -> bool
{
	FileType fileType (type);
	return !fileType.IsFolder () && !fileType.IsRoot ();
};

std::function<bool(long, long)> IsFileInProject =
[](unsigned long state, unsigned long type) -> bool
{
	FileState fileState (state);
	FileType fileType (type);
	return !fileType.IsFolder () && !fileType.IsRoot () && !fileState.IsNone();
};

std::function<bool(long, long)> IsCurrent =
[](unsigned long state, unsigned long type) -> bool
{
	History::ScriptState scriptState (state);
	return scriptState.IsCurrent ();
};

std::function<bool(long, long)> IsRejected =
[](unsigned long state, unsigned long type) -> bool
{
    History::ScriptState scriptState (state);
	return scriptState.IsRejected ();
};

std::function<bool(long, long)> IsLabel =
[](unsigned long state, unsigned long type) -> bool
{
    History::ScriptState scriptState (state);
	return scriptState.IsLabel ();
};

std::function<bool(long, long)> CanSendScript =
[](unsigned long state, unsigned long type) -> bool
{
    History::ScriptState scriptState (state);
	return !scriptState.IsProjectCreationMarker () &&
		   !scriptState.IsInventory () &&
		   !scriptState.IsMissing ();
};

std::function<bool(long, long)> IsCurrentProject =
[](unsigned long state, unsigned long type) -> bool
{
    Project::State project (state);
	return project.IsCurrent ();
};

std::function<bool(long, long)> IsProjectUnavailable =
[](unsigned long state, unsigned long type) -> bool
{
    Project::State project (state);
	return project.IsUnavailable ();
};

std::function<bool(long, long)> CanDeleteScript =
[](unsigned long state, unsigned long type) -> bool
{
	Mailbox::ScriptState script (state);
	return script.IsCorrupted () ||
		   script.IsMissing () ||
		   !script.IsForThisProject () ||
		   script.IsIllegalDuplicate () ||
		   script.IsJoinRequest ();
};

std::function<bool(long, long)> IsMissingScript =
[](unsigned long state, unsigned long type) -> bool
{
	Mailbox::ScriptState script (state);
	return script.IsMissing ();
};

std::function<bool(long, long)> IsProjectRoot =
[](unsigned long state, unsigned long type) -> bool
{
	FileType fileType (type);
	return fileType.IsRoot ();
};

std::function<bool(long, long)> IsDifferent =
[](unsigned long state, unsigned long type) -> bool
{
	MergeStatus status (state);
	return state != 0 && !status.IsIdentical () && !status.IsAbsent ();
};

std::function<bool(long, long)> IsAbsent =
[](unsigned long state, unsigned long type) -> bool
{
	MergeStatus status (state);
	return state != 0 && status.IsAbsent ();
};

std::function<bool(long, long)> IsInteresting =
[](unsigned long state, unsigned long type) -> bool
{
	History::ScriptState scriptState (state);
	return scriptState.IsInteresting ();
};

// File name predicates

bool StartsWithChar::operator () (std::string const & name) const
{
	return ToUpper (name [0]) == _firstChar;
}
