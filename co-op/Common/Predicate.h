#if !defined (PREDICATE_H)
#define PREDICATE_H
//----------------------------------
// (c) Reliable Software 1998 - 2006
//----------------------------------

// State and type predicates used by UI

extern std::function<bool(long, long)> IsStateNone;
extern std::function<bool(long, long)> IsStateInProject;
extern std::function<bool(long, long)> IsCheckedIn;
extern std::function<bool(long, long)> IsCheckoutable;
extern std::function<bool(long, long)> IsFileCheckedIn;
extern std::function<bool(long, long)> IsFolderCheckedIn;
extern std::function<bool(long, long)> IsFolder;
extern std::function<bool(long, long)> IsNewFolder;
extern std::function<bool(long, long)> IsCheckedOutFolderButNotNew;
extern std::function<bool(long, long)> IsNotPresentFolder;
extern std::function<bool(long, long)> IsFolderDeletedBySynch;
extern std::function<bool(long, long)> IsCheckedOut;
extern std::function<bool(long, long)> IsFile;
extern std::function<bool(long, long)> IsFileInProject;
extern std::function<bool(long, long)> IsCurrent;
extern std::function<bool(long, long)> IsRejected;
extern std::function<bool(long, long)> IsLabel;
extern std::function<bool(long, long)> CanSendScript;
extern std::function<bool(long, long)> IsCurrentProject;
extern std::function<bool(long, long)> IsProjectUnavailable;
extern std::function<bool(long, long)> CanDeleteScript;
extern std::function<bool(long, long)> IsMissingScript;
extern std::function<bool(long, long)> IsProjectRoot;
extern std::function<bool(long, long)> IsDifferent;
extern std::function<bool(long, long)> IsAbsent;
extern std::function<bool(long, long)> IsInteresting;

// Name predicates used by UI

class NamePredicate
{
public:
	virtual bool operator () (std::string const & name) const { return false; }
};

class StartsWithChar : public NamePredicate
{
public:
	StartsWithChar (char firstChar)
		: _firstChar (firstChar)
	{}

	bool operator () (std::string const & name) const;

private:
	char	_firstChar;
};

#endif
