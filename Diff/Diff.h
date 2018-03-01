#if !defined DIFF_H
#define DIFF_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "Clustering.h"
#include "CluLines.h"
#include "LineBuf.h"
#include "EditStyle.h"

#include <auto_vector.h>

class DiffRecorder;
namespace Progress { class Meter; }
class ListingWindow;

//
// Compares two versions of a file and clusters lines according to edit action
//
class DifferSource
{
public:
	virtual ~DifferSource () {}

	virtual SimpleLine const * GetNewLine (int i) const = 0;
	virtual SimpleLine const * GetOldLine (int i) const = 0;
	virtual CluSeq GetClusterSeq () const = 0;
	virtual EditStyle::Source GetChangeSource () const = 0;
};

class Differ : public DifferSource, public LineDumper
{
public:
	enum { MaxFileSize =  1000000};  // files above 1 MB are too big

    Differ (char const * bufOld, 
			int lenOld, 
			char const * bufNew, 
			int lenNew, 
			Comparator & comp,
			Progress::Meter & meter,
			EditStyle::Source src = EditStyle::chngNone);

    void Record (DiffRecorder & diffRecorder);
    int NewFileSize () const { return _linesNew.GetSize (); }
    int OldFileSize () const { return _linesOld.GetSize (); }
    void Dump (ListingWindow & listWin, LineCounter & counter, Progress::Meter & meter);
	Clusterer & GetClusterer () { return _clusterer; }
	CluSeq GetClusterSeq () const { return _clusterer.GetClusterSeq (); }
	SimpleLine const * GetNewLine (int i) const { return _linesNew.GetLine (i); }
	SimpleLine const * GetOldLine (int i) const { return _linesOld.GetLine (i); }
	EditStyle::Source GetChangeSource () const { return _src; }

private:	
	enum  // used for optimisation 
	{
		CRITICAL_LINE_COUNT = 30000,
		CRITICAL_SHIFT      =  3000,
		// Above CRITICAL_LINE_COUNT lines, the search simimilar lines is limited to |shift| <= CRITICAL_SHIFT
		THRESHOLD_LINE_COUNT = 5000,
		// This is the ratio of "similars" to total line count,
		// above which we consider a line "bad"
		THRESHOLD_PERCENTAGE = 2,
		// We don't want stretches of bad lines to be longer than that
		MAX_BAD_LINE_STRETCH = 50
	}; 
	void CompareLines (int oldCount, int newCount, Comparator & comp, Progress::Meter & meter);
	void PrepareOptimisation ();
private:
    int					_lenNew;
    int					_lenOld;
    LineBuf				_linesOld;
    LineBuf				_linesNew;
	int					_iStart;
	int					_iTail;
    Clusterer			_clusterer;
    Comparator &		_comp;
	EditStyle::Source	_src;
	std::vector<bool>   _badLines;
};

#endif
