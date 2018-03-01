//----------------------------------
// Reliable Software (c) 1998 - 2005
//----------------------------------

#include "precompiled.h"
#include "CluSum.h"
#include "Diff.h"
#include "ListingWin.h"
#include "DumpWin.h"
#include "LineCounter.h"

#include <Dbg/Assert.h>

#include <iomanip>

ClusterSum::ClusterSum (DifferSource & differ)
	: _newClusters (differ.GetClusterSeq ()),
	  _oldClusters (differ.GetClusterSeq ()),
	  _differ (differ)
{
    _newClusters.SortByNewLineNo ();
    _oldClusters.SortByOldLineNo ();
    // skip insertions in old list
    Cluster const * clu = _oldClusters.GetCur ();
    while (clu != 0)
    {
        if (clu->OldLineNo () != -1)
            break;
        _oldClusters.Advance ();
        clu = _oldClusters.GetCur ();
    }

    // skip deletions in New list
    clu = _newClusters.GetCur ();
    while (clu != 0)
    {
        if (clu->NewLineNo () != -1)
            break;
        _newClusters.Advance ();
        clu = _newClusters.GetCur ();
    }
    _cluNew = _newClusters.GetCur ();
    _cluOld = _oldClusters.GetCur ();
	SelectAction ();
}

EditStyle::Source ClusterSum::GetChangeSource () const 
{ 
	return _differ.GetChangeSource (); 
}

void ClusterSum::SelectAction ()
{
	if (_cluOld != 0 && _cluOld->NewLineNo () == -1)
	{
		_action = EditStyle::actDelete;
		_cluster = _cluOld;
	}
    else if (_cluNew != 0 && _cluNew->OldLineNo () == -1)
	{
		_action = EditStyle::actInsert;
		_cluster = _cluNew;
	}
    else
    {
		// moved
        if (_cluOld != 0 && _cluNew != 0 && _cluOld->OldLineNo () == _cluNew->OldLineNo ())
		{
			_action = EditStyle::actNone;
			_cluster = _cluOld;
		}
        else if (_cluNew == 0 || (_cluOld != 0 && _cluOld->Len () < _cluNew->Len ()))
		{
			_action = EditStyle::actCut;
			_cluster = _cluOld;
		}
		else
        {
            Assert (_cluNew != 0);
			_action = EditStyle::actPaste;
			_cluster = _cluNew;
		}
	}
}

void ClusterSum::Advance ()
{
	if (_action != EditStyle::actPaste && _action != EditStyle::actInsert)
	{
		AdvanceOld ();
	}

	if (_action != EditStyle::actDelete && _action != EditStyle::actCut)
	{
		AdvanceNew ();
	}
	SelectAction ();
}

void ClusterSum::DumpCluster (ListingWindow & listWin, EditStyle::Source src, LineCounter & counter)
{
	Assert (_cluster != 0);
	counter.SetMode (GetAction (), _cluster->NewLineNo ());
	EditStyle edit (src, GetAction ());
	for (LineIter it (*this); !it.AtEnd (); it.Advance ())
	{
		SimpleLine const * line = it.GetLine ();
		listWin.PutLine (line->Buf (), line->Len (), edit, counter.NextLineNo ());
	}
}

void ClusterSum::DumpCluster (DumpWindow & dumpWin, EditStyle::Source src, LineCounter & counter)
{
	Assert (_cluster != 0);
	//
	// Cluster dump format:
	//
	// Old     New
	// line    line     Contents or number of unchaged lines
	// no.     no.
	//
	//   1       1      16
	//  17      --      deleted line
	//  --      17      added line
	//
	counter.SetMode (GetAction (), _cluster->NewLineNo ());
	std::ostringstream lineInfo;
	if (_cluster->OldLineNo () != -1 && _cluster->NewLineNo () != -1)
	{
		lineInfo << std::setw (5) << std::setfill (' ') << std::right << _cluster->OldLineNo ();
		lineInfo << "   ";
		lineInfo << std::setw (5) << std::setfill (' ') << std::right << _cluster->NewLineNo ();
		lineInfo << "   ";
		lineInfo << std::setw (5) << std::setfill (' ') << std::right << _cluster->Len () << " line(s)" << std::endl;
	}	
	
	EditStyle edit (src, GetAction ());
	for (LineIter it (*this); !it.AtEnd (); it.Advance ())
	{
		SimpleLine const * line = it.GetLine ();
		if (line->Len () == -1) // special skip line
		{
			dumpWin.PutLine (lineInfo.str ().c_str (), EditStyle ());
			break;
		}
		std::ostringstream dumpLine;
		if (it.IsFirstClusterLine ())
		{
			if (GetAction () == EditStyle::actDelete)
			{
				Assert (_cluster->OldLineNo () != -1);
				dumpLine << std::setw (5) << std::setfill (' ') << std::right << _cluster->OldLineNo ();
				dumpLine << "      --   ";
			}
			else
			{
				Assert (GetAction () == EditStyle::actInsert);
				Assert (_cluster->NewLineNo () != -1);
				dumpLine << "   --   ";
				dumpLine << std::setw (5) << std::setfill (' ') << std::right << _cluster->NewLineNo ();
				dumpLine << "   ";
			}
		}
		dumpLine << line->Buf ();
		dumpWin.PutLine (dumpLine.str ().c_str (), edit, it.IsFirstClusterLine ());
	}
}

void ClusterSum::DumpCluster (ListingWindow & listWin, EditStyle::Source src, LineCounter & counter, std::set<int> & cluProcessed)
{
	EditStyle::Action act = GetAction ();
	if (_cluster->OldLineNo () != -1)
	{
		if (act == EditStyle::actNone)
		{
			if (cluProcessed.find (_cluster->OldLineNo ()) != cluProcessed.end ())
				return;
		}
		cluProcessed.insert (_cluster->OldLineNo ());
	}

	counter.SetMode (act, _cluster->NewLineNo ());
	EditStyle edit (src, act);
	for (LineIter it (*this); !it.AtEnd (); it.Advance ())
	{
		SimpleLine const * line = it.GetLine ();
		listWin.PutLine (line->Buf (), line->Len (), edit, counter.NextLineNo ());
	}
}


DiffSum::DiffSum (Differ & diffUser, Differ & diffSynch)
	: _cluSumUser (diffUser), 
	  _cluSumSynch (diffSynch),
	  _cluSum (0)
{
	Init ();
}

void DiffSum::Advance ()
{
	Cluster const * cluUser = _cluSumUser.GetCluster ();
	Cluster const * cluSynch = _cluSumSynch.GetCluster ();
	if (cluUser != 0 && cluSynch != 0 &&
		cluUser->OldLineNo () == cluSynch->OldLineNo () &&
		cluUser->OldLineNo () != -1)
	{
		_cluSumUser.Advance ();
		_cluSumSynch.Advance ();
	}
	else
		_cluSum->Advance ();

	Init ();
}

void DiffSum::Init ()
{
	EditStyle::Action actUser = EditStyle::actInvalid;
	EditStyle::Action actSynch = EditStyle::actInvalid;
	if (!_cluSumUser.AtEnd ())
		actUser = _cluSumUser.GetAction ();
	if (!_cluSumSynch.AtEnd ())
		actSynch = _cluSumSynch.GetAction ();
	// First check for valid action	
    if (actUser == EditStyle::actInvalid)
	{
		_cluSum = &_cluSumSynch;
	}
	else if (actSynch == EditStyle::actInvalid)
	{
		_cluSum = &_cluSumUser;
	}
	 
	else
	{
		// check for lated None clusters
		int oldUser = _cluSumUser.GetCluster ()->OldLineNo ();
		int oldSynch = _cluSumSynch.GetCluster ()->OldLineNo ();
		if (oldUser < oldSynch && actUser == EditStyle::actNone && actSynch != EditStyle::actPaste)
		{
			_cluSum = &_cluSumUser;
		}
		else if (oldSynch < oldUser && actSynch == EditStyle::actNone && actUser != EditStyle::actPaste)
		{
			_cluSum = &_cluSumSynch;
		}
		//check for deleted and new clusters
		else if (actUser == EditStyle::actDelete)
		{				
			_cluSum = &_cluSumUser;
		}
		else if (actSynch == EditStyle::actDelete)
		{		
			_cluSum = &_cluSumSynch;
		}
		else if (actUser == EditStyle::actInsert)
		{
			_cluSum = &_cluSumUser;
		}
		else if (actSynch == EditStyle::actInsert)
		{
			_cluSum = &_cluSumSynch;
		}
		else // both moved
		{
			// cut or paste?
			if (actUser == EditStyle::actCut)
				_cluSum = &_cluSumUser;
			else if (actSynch == EditStyle::actCut)
				_cluSum = &_cluSumSynch;
			else if (actUser == EditStyle::actPaste)
				_cluSum = &_cluSumUser;
			else if (actSynch == EditStyle::actPaste)
				_cluSum = &_cluSumSynch;
			else // anything else
				_cluSum = &_cluSumUser;
		}
	}
}

void DiffSum::DumpCluster (ListingWindow & listWin, LineCounter & counter)
{
	_cluSum->DumpCluster (listWin, _cluSum->GetChangeSource (), counter, _cluProcessed);
}

void DiffSum::DumpCluster (std::vector<char> & buf)
{
	_cluSum->DumpCluster (buf, _cluProcessed);
}

void ClusterSum::DumpCluster (std::vector<char> & buf, std::set<int> & cluProcessed)
{
	EditStyle::Action act = GetAction ();
	if (_cluster->OldLineNo () != -1)
	{
		if (act != EditStyle::actPaste)
		{
			if (cluProcessed.find (_cluster->OldLineNo ()) != cluProcessed.end ())
				return;
		}
		cluProcessed.insert (_cluster->OldLineNo ());
	}
		
	if (act == EditStyle::actDelete || act == EditStyle::actCut)
		return;
	for (LineIter it (*this); !it.AtEnd (); it.Advance ())
	{
		SimpleLine const * line = it.GetLine ();
		buf.insert ( buf.end (),line->Buf (), line->Buf () +  line->Len ());
	}
}

LineIter::LineIter (ClusterSum & cluSum)
	: _cluster (cluSum.GetCluster ()), 
	  _count (_cluster->Len ()),
	  _action (cluSum.GetAction ()),
	  _differ (cluSum._differ),
	  _curCount (0),
	  _line (0)
{
	if (_action != EditStyle::actDelete)
		_targetLineNo = _cluster->NewLineNo ();
	InitLine ();
}

void LineIter::Advance ()
{
	_curCount++;
	InitLine ();
}

void LineIter::InitLine ()
{
	if (AtEnd ())
		return;

	if (_action == EditStyle::actNone || _action == EditStyle::actInsert || _action == EditStyle::actPaste)
		_line = _differ.GetNewLine (_cluster->NewLineNo () + _curCount);
	else if (_action == EditStyle::actDelete || _action == EditStyle::actCut)
		_line = _differ.GetOldLine (_cluster->OldLineNo () + _curCount);
}
