#if !defined (ATOM_H)
#define ATOM_H
//
// (c) Reliable Software 1999, 2000
//
//////////////////////////////////////////////////////

namespace Win
{
	class GlobalAtom
	{
	public:
		class String: public std::string
		{
		public:
			String (unsigned atom);
		};
	public:
		GlobalAtom (std::string const & str)
		{
			_atom = ::GlobalAddAtom (str.c_str ());
		}
		~GlobalAtom ()
		{
			if (IsOK ())
				::GlobalDeleteAtom (_atom);
		}

		bool IsOK () const { return _atom != 0; }
		unsigned short GetAtom () const { return _atom; }
	private:
		ATOM _atom;
	};
}

#endif
