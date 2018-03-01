#if !defined (MERGE_H)
#define MERGE_H
//----------------------------------
// Reliable Software (c) 1998 - 2007
//----------------------------------

#include "LineBuf.h"

class Differ;
class Clusterer;
template <class T> class DEditListIter;
namespace Progress { class Meter; }

class Merger: public LineDumper
{
public:
	Merger (Differ & diffUser, Differ & diffSynch, Progress::Meter & meter);
    void Dump (ListingWindow & dumpWin, LineCounter & counter, Progress::Meter & meter);
	void Dump (std::vector<char> & buf);

private:
	void MergeDiffs (Progress::Meter & meter);

private:
	Differ &	_diffUser;
	Clusterer & _userClusterer;
	Differ &	_diffSynch;
	Clusterer & _synchClusterer;
};

#endif
