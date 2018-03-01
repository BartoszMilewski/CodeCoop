#if !defined (MAPICONTENTSTABLE_H)
#define MAPICONTENTSTABLE_H
//--------------------------------
// (c) Reliable Software 2001-2003
//--------------------------------

namespace Mapi
{
	class Folder;

	class ContentsTable
	{
	public:
		ContentsTable (Folder & folder, bool unreadOnly);

		unsigned int size () const { return _ids.size (); }
		std::vector<unsigned char> const & operator [] (unsigned int i) const { return _ids [i]; }

	private:
		enum
		{
			RowsPerQuery = 50
		};

	private:
		std::vector<std::vector<unsigned char> >	_ids;
	};
}

#endif
