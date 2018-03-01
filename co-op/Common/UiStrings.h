#if !defined (UISTRINGS_H)
#define UISTRINGS_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

char const SelectBranch []	= "<Select branch...>";	// Fake entry in the merge target hint list
char const NoTarget []	= "No target selected";		// Fake entry in the merge target hint list
char const CurrentProject [] = "Current project";
struct MergeTypeEntry
{
	char const * _dispName;
	bool const   _isAncestor;
	bool const   _isCumulative;
};

MergeTypeEntry const MergeTypeEntries [] = 
{ 
	{ "Selective", false, false },
	{ "Common Ancestor", true, false },
	{ "Cumulative", true, true },
	{ 0 , false, false }
};

#endif
