#if !defined (ENCRYPTION_H)
#define ENCRYPTION_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

class Catalog;

namespace Encryption
{
	class KeyMan
	{
	public:
		KeyMan (Catalog & cat, std::string const & projName);

		std::string const & GetKey () const { return _projKey; }
		void SetKey (std::string const & key);

		std::string const & GetCommonKey () const { return _commonKey; }

	private:
		Catalog		&       _cat;
		std::string		    _projKey;
		std::string         _commonKey;
	};
};

#endif
