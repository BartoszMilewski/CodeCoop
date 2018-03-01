#if !defined (LINEAGE_H)
#define LINEAGE_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "GlobalId.h"
#include "Serialize.h"

#include <iosfwd>

namespace Unit
{
	enum Type
	{
		Set = 0,		// not a unit
		Member = 1,
		File = 2,
		Milestone = 3,
		SubProject = 4,
		Ignore = 5,		// Ignore unit type -- used by control and package scripts
		LastUnit = 6	// Must be always last in a sequence! 
	};

	bool VerifyType (Type type);

	class ScriptId : public Serializable
	{
	public:
		ScriptId (GlobalId scriptId, Unit::Type unitType)
			: _scriptId (scriptId),
			  _unitType (unitType)
		{}
		ScriptId (Deserializer& in, int version)
		{
			Deserialize (in, version);
		}

		GlobalId Gid () const { return _scriptId; }
		Unit::Type Type () const { return _unitType; }

		// Serializable interface

		void Serialize (Serializer& out) const;
		void Deserialize (Deserializer& in, int version);

	private:
		GlobalId	_scriptId;
		Unit::Type	_unitType;
	};

	typedef std::vector<Unit::ScriptId> ScriptList;

	class Sequencer
	{
	public:
		Sequencer ()
			: _cur (0)
		{}
		bool AtEnd () const  { return _cur == LastUnit; }
		void Advance ()      { _cur++; }

		Type GetUnitType () const { return static_cast<Type>(_cur); }

	private:
		int	_cur;
	};

}

class Lineage : public Serializable
{
public:
	enum CmpResult
	{
		Equal,
		Longer,
		Shorter,
		Higher,  // priority
		Lower
	};

	class Sequencer
	{
	public:
		Sequencer (Lineage const & lineage)
			: _cur (lineage._gidList.begin ()),
			  _end (lineage._gidList.end ())
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		GlobalId GetScriptId () const { return *_cur; }
		GidList::const_iterator CurrentIter () const { return _cur; }
		GidList::const_iterator EndIter () const { return _end; }

	private:
		GidList::const_iterator	_cur;
		GidList::const_iterator	_end;
	};

	class ReverseSequencer
	{
	public:
		ReverseSequencer (Lineage const & lineage)
			: _cur (lineage._gidList.rbegin ()),
			  _end (lineage._gidList.rend ())
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		GlobalId GetScriptId () const { return *_cur; }

	private:
		GidList::const_reverse_iterator	_cur;
		GidList::const_reverse_iterator	_end;
	};

	friend class Sequencer;
	friend class ReverseSequencer;
	typedef GidList::const_iterator const_iterator;
	typedef GidList::iterator iterator;

public:
	Lineage ()
	{}
	Lineage (Deserializer& in, int version)
	{
		Deserialize (in, version);
	}
    Lineage (Lineage const & lineage) { ReInit (lineage); }
	Lineage (GidList const & gidList) : _gidList (gidList) {}
	void InitReverse (GidList const & reverseLineage);
    void ReInit (Lineage const & lineage);
    void Clear () { _gidList.clear (); }
	void swap (Lineage & lineage) { _gidList.swap (lineage._gidList); }
    unsigned int Count () const { return _gidList.size (); }
    GlobalId GetReferenceId () const 
    { 
        return (_gidList.size () > 0)? _gidList [0]: gidInvalid; 
    }
    GlobalId GetLastScriptId () const 
    { 
        return (_gidList.size () > 0)? _gidList [_gidList.size () - 1]: gidInvalid; 
    }

    void PushId (GlobalId gid) { _gidList.push_back (gid); }
	void PopBack () { _gidList.pop_back (); }

    GlobalId operator [] (unsigned int i) const 
    { 
        Assert (i < _gidList.size ());
        return _gidList [i]; 
    }

    CmpResult CompareWith (Lineage const & lin, int & idxDiff) const;
	void Append (GidList::const_iterator begin, GidList::const_iterator end);

    // Serializable interface

    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);

	bool Verify () const;

	const_iterator begin () const { return _gidList.begin (); }
	const_iterator end () const { return _gidList.end (); }
	iterator begin () { return _gidList.begin (); }
	iterator end () { return _gidList.end (); }

protected:
	GidList		_gidList;	// Script global id list
};

class UnitLineage : public Lineage
{
public:
	UnitLineage (Lineage const & lineage, Unit::Type type, GlobalId unitId)
		: Lineage (lineage),
		  _type (type),
		  _unitId (unitId)
	{}
	UnitLineage (Unit::Type type, GlobalId unitId)
		: _type (type),
		  _unitId (unitId)
	{}
	UnitLineage (Deserializer& in, int version)
	{
		Deserialize (in, version);
	}

	Unit::Type GetUnitType () const { return _type; }
	GlobalId GetUnitId () const { return _unitId; }

	enum Type
	{
		Empty,	// No side lineages in header
		Minimal,// Minimal (only not-confirmed script ids) side lineages in header
		Maximal	// Complete (including confirmed script ids) lineage in header
	};

    // Serializable interface

    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);

	bool Verify () const;

public:
	class Sequencer : public Lineage::Sequencer
	{
	public:
		Sequencer (UnitLineage const & lineage)
			: Lineage::Sequencer (lineage),
			  _type (lineage._type),
			  _unitId (lineage._unitId)
		{}

		Unit::Type GetUnitType () const { return _type; }
		GlobalId GetUnitId () const { return _unitId; }

	private:
		Unit::Type	_type;
		GlobalId	_unitId;
	};

	friend class Sequencer;

private:
	Unit::Type	_type;
	GlobalId	_unitId;
};

std::ostream & operator<<(std::ostream & os, Lineage const & lineage);
std::ostream & operator<<(std::ostream & os, UnitLineage const & lineage);
std::ostream & operator<<(std::ostream & os, Unit::Type type);
std::ostream & operator<<(std::ostream & os, Unit::ScriptId id);

#endif
