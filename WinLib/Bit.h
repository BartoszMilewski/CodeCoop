#if !defined (BIT_H)
#define BIT_H
//---------------------------------------
//  (c) Reliable Software, 2003
//---------------------------------------
#include <bitset>

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
	bool IsEmpty () const { return _value == 0; }
protected:
	unsigned long	_value;
};

// BitEnum has power of two values
template<class BitEnum>
class BitFieldMask: public BitFieldBase
{
public:
	BitFieldMask (unsigned long value = 0) : BitFieldBase (value) {}
	bool IsSet (BitEnum test) const { return BitFieldBase::IsSet (test); }
	void Set (BitEnum bit) { BitFieldBase::Set (bit); }
	void Clear (BitEnum bit) { BitFieldBase::Clear (bit); }
	void Union (BitFieldMask<BitEnum> bits) 
	{
		_value |= bits;
	}
	void Difference (BitFieldMask<BitEnum> bits)
	{
		_value &= ~bits;
	}
};

template<class BitIndexEnum>
class BitSet
{
public:
	bool test (BitIndexEnum idx) const { return _set.test (idx); }
	void set (BitIndexEnum idx, bool val = true) { _set.set (idx, val); }
	unsigned long to_ulong () const { return _set.to_ulong (); }
	void init (unsigned long set) { _set = static_cast<unsigned long long>(set); }
	bool empty () const { return _set.to_ulong () == 0; }
	void clear() { _set.reset(); }
private:
	std::bitset<std::numeric_limits<unsigned long>::digits> _set;
};

class BitTree
{
public:
	BitTree ()
		: _level (0),
		  _mask (0),
		  _bits (0)
	{}

	void Branch (bool bit)
	{
		_mask |= (1 << _level);
		if (bit)
			_bits |= (1 << _level);
		++_level;
	}

	bool Match (unsigned long bits) const
	{
		return (bits & _mask) == _bits; 
	}

private:
	unsigned long	_level;
	unsigned long	_mask;
	unsigned long	_bits;
};

#endif
