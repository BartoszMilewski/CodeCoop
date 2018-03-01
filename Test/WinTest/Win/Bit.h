#if !defined (BIT_H)
#define BIT_H
//---------------------------------------
//  (c) Reliable Software, 2003
//---------------------------------------

class BitFieldBase
{
public:
	BitFieldBase (unsigned long value = 0)
		: _value (value)
	{}
	operator unsigned long () const { return _value; }
	bool IsSet (unsigned long test) const { return (_value & test) != 0; }
	void Set (unsigned long bit) { _value |= bit; }
	void Clear (unsigned long bit) { _value &= ~bit; }
	void ReSet () { _value = 0; }
protected:
	unsigned long	_value;
};

template<class BitEnum>
class BitField: public BitFieldBase
{
public:
	BitField (unsigned long value = 0) : BitFieldBase (value) {}
	bool IsSet (BitEnum test) const { return BitFieldBase::IsSet (test); }
	void Set (BitEnum bit) { BitFieldBase::Set (bit); }
	void Clear (BitEnum bit) { BitFieldBase::Clear (bit); }
};

#endif
