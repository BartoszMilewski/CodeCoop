#if !defined (CLUSUM_H)
#define CLUSUM_H
//-----------------------------------
// Reliable Software (c) 1998 -- 2002
//-----------------------------------

#include "Clustering.h"
#include "EditStyle.h"

class Differ;
class DifferSource;
class LineCounter;
class SimpleLine;
class ListingWindow;
class DumpWindow;

class ClusterSum
{
	friend class LineIter;
public:
	ClusterSum (DifferSource & differ);
	bool AtEnd () const {  return _cluster == 0; }
	void Advance ();
	int TotalCount () const { return _newClusters.Count () + _oldClusters.Count (); }

	Cluster const * GetCluster () const { return _cluster; }
	EditStyle::Action GetAction () const { return _action; }
    void DumpCluster (ListingWindow & listWin, EditStyle::Source src, LineCounter & counter);
    void DumpCluster (DumpWindow & fileWin, EditStyle::Source src, LineCounter & counter);
	void DumpCluster (ListingWindow & listWin, EditStyle::Source src, LineCounter & counter, std::set<int> & cluProcessed);
    void DumpCluster (std::vector<char> & buf, std::set<int> & cluProcessed);
	EditStyle::Source GetChangeSource () const;

private:
	void SelectAction ();
	void AdvanceOld () 
	{ 
		_oldClusters.Advance ();
		_cluOld = _oldClusters.GetCur ();
	}
	void AdvanceNew () 
	{ 
		_newClusters.Advance ();
		_cluNew = _newClusters.GetCur ();
	}
private:
	DifferSource  &		_differ;
	ClusterSort			_newClusters;
	ClusterSort			_oldClusters;
	Cluster const *		_cluster;
    Cluster const *		_cluOld;
    Cluster const *		_cluNew;
	EditStyle::Action	_action;
};

class DiffSum
{
public:
	DiffSum (Differ & diffUser, Differ & diffSynch);
	bool AtEnd () const {  return _cluSum->AtEnd (); }
	void Advance ();
    void DumpCluster (ListingWindow & listWin, LineCounter & counter);
    void DumpCluster (std::vector<char> & buf);

private:
	void Init ();

private:
	ClusterSum		_cluSumUser;
	ClusterSum		_cluSumSynch;

	ClusterSum *	_cluSum;
	std::set<int>	_cluProcessed;;
};

class LineIter
{
public:
	LineIter (ClusterSum & cluSum);
	bool AtEnd () { return _curCount == _count; }
	void Advance ();
	SimpleLine const * GetLine () const { return _line; }
	int GetTargetLineNo () const { return _targetLineNo; }
	bool IsFirstClusterLine () const { return _curCount == 0; }

private:
	void InitLine ();

private:
	DifferSource &			_differ;
	Cluster const *			_cluster;
	EditStyle::Action const	_action;
	int	const				_count;
	int						_curCount;
	SimpleLine const *		_line;
	int						_targetLineNo;
};

#endif
