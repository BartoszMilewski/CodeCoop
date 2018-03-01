#if !defined (FILESTATE_H)
#define FILESTATE_H
//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "Serialize.h"
#include "Area.h"

#include <iosfwd>

class FileState: public Serializable
{
public:
    FileState ()
		: _value (0)
	{}
    FileState (FileState const & state) 
    { 
        _value = state._value;
    }
    FileState (unsigned long value)
        : _value (value)
    {}
    void operator= (FileState const & state) 
    { 
        _value = state._value;
    }
    unsigned long GetValue () const { return _value; } 
    char const * GetName () const ;
    void Reset () 
    { 
        _value = 0; 
    }

	bool IsEqual (FileState const & state) const
	{
		return _value == state._value;
	}
    void SetCoDelete (bool bit) { _bits._coDelete = bit; }
    void SetSoDelete (bool bit) { _bits._soDelete = bit; }
	void SetMerge (bool bit) { _bits._M = bit; }
    void SetCoDiff (bool bit) { _bits._coDiff = bit; }
    void SetSoDiff (bool bit) { _bits._soDiff = bit; }
	void SetMergeConflict (bool bit) { _bits._Mc = bit; }
	void SetResolvedNameConflict (bool bit) { _bits._Rnc = bit; }
	void SetRenamed (bool bit) { _bits._Rn = bit; }
	void SetTypeChanged (bool bit) { _bits._TypeCh = bit; }
	void SetMoved (bool bit) { _bits._Mv = bit; }
	void SetCheckedOutByOthers (bool bit) { _bits._Out = bit; } 

    bool IsNone () const { return (_value & 0x1ff) == 0; }
	bool IsDirtyUncontrolled () const;
	bool IsCheckedIn () const { return IsPresentIn (Area::Project) && !IsRelevantIn (Area::Original); }

    bool IsCoDelete () const { return _bits._coDelete != 0; }
    bool IsSoDelete () const { return _bits._soDelete != 0; }

    bool IsCoDiff () const { return _bits._coDiff != 0; }
    bool IsSoDiff () const { return _bits._soDiff != 0; }
	bool IsResolvedNameConflict () const { return _bits._Rnc != 0; }
	bool IsRenamed () const { return _bits._Rn != 0; }
	bool IsTypeChanged () const { return _bits._TypeCh != 0; }
	bool IsMoved () const { return _bits._Mv != 0; }
	bool IsCheckedOutByOthers () const { return _bits._Out != 0; }

	bool IsMerge () const { return _bits._M != 0; }
	bool IsMergeConflict () const { return _bits._Mc != 0; }
	bool IsMergeContent () const
	{
		return IsMerge () &&
			   IsPresentIn (Area::Project) &&
			   IsPresentIn (Area::Original) &&
			   IsPresentIn (Area::Synch);
	}

	// Is new in the project because of add or restore delete operation
    bool IsNew () const             
    { 
        return IsPresentIn (Area::Project) &&
			  !IsPresentIn (Area::Original) && 
			   IsRelevantIn (Area::Original);
    }

	bool IsToBeDeleted () const
    {
        return !IsPresentIn (Area::Project) && IsRelevantIn (Area::Original);
    }

    bool IsSynchDelete () const
    {
        return IsRelevantIn (Area::Synch) && !IsPresentIn (Area::Synch);
    }

	bool WillBeRestored () const
	{
		return IsPresentIn (Area::Reference) && IsRelevantIn (Area::Reference);
	}

	bool IsPresentIn (Area::Location loc) const;
	void SetPresentIn (Area::Location loc, bool val);
	bool IsRelevantIn (Area::Location loc) const;
	void SetRelevantIn (Area::Location loc, bool val);

    void Serialize (Serializer & out) const;
    void Deserialize (Deserializer & in, int version);

private:
	class StateName
	{
	public:
		unsigned char	bits [13];
		char const *	str;
	};

	bool IsBitEqual (unsigned stateBit, unsigned maskBit) const
	{
		return maskBit == 2 || stateBit == maskBit;
	}

private:
    union
    {
        unsigned long _value;       // for quick serialization
        struct
        {
			// Notice: there are explicit bit masks based on this layount in IsNone and StateBrandNew
            unsigned long _P:1;     // 1 -- present in the project area

			unsigned long _PS:1;	// 1 -- present in the pre-synch area
			unsigned long _pso:1;	// 1 -- relevant in the pre-synch out

            unsigned long _O:1;     // 1 -- present in the original area
            unsigned long _co:1;    // 1 -- relevant in the original area

            unsigned long _R:1;     // 1 -- present in the reference area
			unsigned long _ro:1;	// 1 -- relevant in the referenced out

            unsigned long _S:1;     // 1 -- present in the synch area
            unsigned long _so:1;    // 1 -- relevant in the synched out

            unsigned long _coDelete:1;  // 1 -- checked out to be physically deleted
            unsigned long _soDelete:1;  // 1 -- synched out to be physically deleted

            // volatile members
            unsigned long _coDiff:1;// 1 -- checked out different from original
            unsigned long _soDiff:1;// 1 -- synched out different from reference

			// back to persistent members
			unsigned long _M:1;		// 1 -- merge requiring user intervention
			unsigned long _Mc:1;	// 1 -- automatic merge failed because of conflict

			// volatile members
			unsigned long _Rnc:1;	// 1 -- resolved name conflict -- in the project area file still uses old (conflict) name,
									//      while in the database we already have stored its new (non-conflict) name.
			unsigned long _Rn:1;	// 1 -- renamed -- used only for display
			unsigned long _Mv:1;	// 1 -- moved -- used only for display
			unsigned long _Out:1;	// 1 -- checked out by others - used only for display
			unsigned long _TypeCh:1;// 1 -- file type changed
        } _bits;
    };

	static StateName const _name [];
};

class StateBrandNew : public FileState
{
public:
	StateBrandNew ()
		: FileState (0x11)
	{}
};

class StateCheckedIn : public FileState
{
public:
	StateCheckedIn ()
		: FileState (0x1)
	{}
};

std::ostream & operator<<(std::ostream & os, FileState fs);

#endif
