//--------------------------------------
// (c) Reliable Software 2001
//-------------------------------------

#include "precompiled.h"
#include "Search.h"
#include "Registry.h"

SearchRequest::SearchRequest ()
		:_directionForward (true)
{
	Registry::UserDifferPrefs regPrefs;
	regPrefs.GetFindPref (_wholeWord, _matchCase);
	std::string findWord = regPrefs.GetFindWord ();	
	SetFindWord (findWord);
}
