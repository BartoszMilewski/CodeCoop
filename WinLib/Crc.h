#if !defined (CRC_H)
#define CRC_H
//--------------------------------
//  (c) Reliable Software, 2001-03
//--------------------------------

namespace Crc
{
	typedef unsigned long Type;

	// The CRC key is fixed in calculating the table

	class Calc
	{
	public:
		Calc () : _register (0) {}
		void PutByte (unsigned byte);
		void PutBuffer (unsigned char const * buf, unsigned long size);
		Type Done ()
		{
			Type tmp = _register;
			_register = 0;
			return tmp;
		}
	private:
		Type _register;
		static Type _table [256];
	};


	inline void Calc::PutByte (unsigned byte)
	{
		Assert ((byte & 0xffffff00) == 0);
		unsigned top = _register >> 24;
		top ^= byte;
		_register = (_register << 8) ^ _table [top];
	}
}

#endif
