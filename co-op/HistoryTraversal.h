#if !defined (HISTORYTRAVERSAL_H)
#define HISTORYTRAVERSAL_H
//----------------------------------
// (c) Reliable Software 2004 - 2007
//----------------------------------

#include "GlobalId.h"
#include "HistorySortedTree.h"

namespace History
{

	// ProcessNode is called for every node up and including a branching point
	// Then Push is called with the branching point id and a new branch is processed
	// After the end of branch is processed, BackTo is called with the branchId
	// of the origin of the next branch to be processed (gidInvalid, if end of tree)
	// Example: the sequence when processing a branch point X with 3 branches:
	// - ProcessNode with the branching point node X
	// - Push with the id of the branching point X
	// - ProcessNode for all the nodes of the first branch
	// - BackTo with the id of X
	// - ProcessNode for all the nodes of the second branch
	// - BackTo with the id of X
	// - ProcessNode for all the nodes of the third branch
	// - BackTo with the id of an earlier branching point
	//   or with gidInvalid if the end of tree reached.
	// Note: we also have to gather side lineages of dead members!

	class Gatherer
	{
	public:
		virtual ~Gatherer () {}
		// return false to terminate traversal
		virtual bool ProcessNode (SortedTree::XFwdUnitSequencer & seq) = 0;
		virtual void Push (GlobalId branchId) = 0;
		// return false to terminate traversal
		virtual bool BackTo (GlobalId branchId) = 0;
	};

	void XTraverseTree (SortedTree::XFwdUnitSequencer & seq, Gatherer & gatherer);


	class SideLineageGatherer : public Gatherer
	{
	public:
		SideLineageGatherer (Unit::Type unitType,
							 unsigned unitId,
							 ScriptHeader & hdr,
							 UnitLineage::Type sideLineageType) 
			: _unitType (unitType),
			  _unitId (unitId),
			  _sideLineageType (sideLineageType),
			  _hdr (hdr),
			  _lastReferenceId (gidInvalid)
		{}

		bool ProcessNode (SortedTree::XFwdUnitSequencer & seq);
		void Push (GlobalId branchPtId);
		bool BackTo (GlobalId branchId);


	private:
		void Accumulate (Lineage & lin);

	private:
		class Segment
		{
		public:
			Segment (GlobalId branchingPoint, bool isConfirmed)
				: _branchPoint (branchingPoint),
				  _isBranchPointConfirmed (isConfirmed)
			{}
			void push_back (GlobalId gid)
			{
				_segmentIds.push_back (gid);
			}
			GlobalId GetBranchPtId () const { return _branchPoint; }
			bool IsBranchPtConfirmed () const { return _isBranchPointConfirmed; }
			GidList::const_iterator begin () const { return _segmentIds.begin (); }
			GidList::const_iterator end () const { return _segmentIds.end (); }
			void clear () { _segmentIds.clear (); }

		private:
			GlobalId	_branchPoint;
			bool		_isBranchPointConfirmed;
			// from (but excluding) the branching point up to (including) next branching point
			GidList		_segmentIds;
		};

	private:
		Unit::Type			_unitType;
		unsigned			_unitId;
		UnitLineage::Type	_sideLineageType;
		GidList				_stem; // before and including the first branching point
		GlobalId			_lastReferenceId;

		std::vector<Segment> _stack;
		ScriptHeader &		_hdr;
	};

	class RepairGatherer : public Gatherer
	{
		friend class IsEqualBranchId;

	public:
		RepairGatherer (SortedTree & tree); 

		bool ProcessNode (SortedTree::XFwdUnitSequencer & seq);
		void Push (GlobalId branchPtId);
		bool BackTo (GlobalId branchId);

		bool TreeChanged () const { return _treeChanged; }

	private:
		typedef std::vector<unsigned>::const_iterator IdxIterator;

		class Segment
		{
		public:
			Segment (GlobalId branchingPoint, bool doRemove)
				: _branchPoint (branchingPoint),
				  _doRemove (doRemove)
			{}
			void push_back (unsigned idx)
			{
				_segmentIdx.push_back (idx);
			}
			void SetDoRemove (bool flag) { _doRemove = flag; }
			GlobalId GetBranchPtId () const { return _branchPoint; }
			bool IsDoRemove () const { return _doRemove; }
			bool IsEmpty () const { return _segmentIdx.size () == 0; }
			void clear () { _segmentIdx.clear (); }
			IdxIterator begin () const { return _segmentIdx.begin (); }
			IdxIterator end () const { return _segmentIdx.end (); }

		private:
			GlobalId	_branchPoint;
			bool		_doRemove;
			// tree indexes from (but excluding) the branching point up to (including) next branching point
			std::vector<unsigned>	_segmentIdx;
		};

	private:
		SortedTree &		_tree;
		GidSet				_visited;
		std::vector<Segment> _stack;
		bool				_doRemove;
		bool				_treeChanged;
	};

}

#endif
