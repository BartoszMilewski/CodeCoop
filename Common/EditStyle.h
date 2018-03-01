#if !defined (EDITSTYLE_H)
#define EDITSTYLE_H
//------------------------------------------------
// EditStyle.h
// (c) Reliable Software 2002
// -----------------------------------------------

class EditStyle
{
public:
	enum Action
	{
		actNone,
		actDelete,
		actInsert,
		actCut,
		actPaste,
		actInvalid
	};

	enum Source
	{
		chngNone,		// No changes
		chngHistory,	// Historical change
		chngUser,		// User change
		chngSynch,		// Synch change
		chngMerge		// Merge change
	};

    EditStyle (unsigned short value = 0)
		: _value (value)
	{}
	EditStyle (Source source, Action action);
    EditStyle (EditStyle const & style) 
    { 
        _value = style._value;
    }
    void operator= (EditStyle const & style) 
    { 
        _value = style._value;
    }
	bool operator!= (EditStyle const & style)
	{
		return  _value != style._value;
	}
	Action GetAction () const;
	Source GetChangeSource () const { return static_cast<Source> (_bits._change); }
	bool IsMoved () const { return _bits._moved != 0; }
	bool IsRemoved ()  const { return _bits._removed != 0; }
	bool IsChanged () const { return _bits._change != chngNone; }
	unsigned short GetValue () const { return _value; }

private:

    union
    {
        unsigned short _value;			// for quick assignment
        struct
        {
            unsigned short _moved:1;	// Line moved
			unsigned short _removed:1;	// Line deleted/cut
			unsigned short _change:3;	// Change source
        } _bits;
    };
};

class UserInsertStyle : public EditStyle
{
public:
	UserInsertStyle ()
		: EditStyle (chngUser, actInsert) {}
};

#endif
