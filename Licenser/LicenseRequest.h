#if !defined (LICENSEREQUEST_H)
#define LICENSEREQUEST_H
//---------------------------
// (c) Reliable Software 2005
//---------------------------

#include <File/Path.h>
class LicenseRequest
{
public:
	enum Type
	{
		Simple, Distributor, Block
	};

	LicenseRequest () 
		: _licCount (0), 
		  _startNum (0), 
		  _seatCount (0),
		  _product ('P'),
		  _bcSeatCount (0), 
		  _price (0), 
		  _type (Simple),
		  _preview (true) // preview by default
	{}
	void Clear ()
	{
		// leave template file unchanged
		_comment.clear ();
		_email.clear ();
		_licensee.clear ();
		_licCount = 0;
		_startNum = 0;
		_seatCount = 0;
		_product = 'P';
		_bcSeatCount = 0;
		_price = 0;
		// leave type unchanged
		// leave preview unchanged;
	}
	std::string & Email () { return _email; }
	std::string & Licensee () { return _licensee; }
	int & LicenseCount () { return _licCount; }
	int & Seats () { return _seatCount; }
	char & Product () { return _product; }
	int & StartNum () { return _startNum; }
	int & BCSeats () { return _bcSeatCount; }
	std::string & Comment () { return _comment; }
	int & Price () { return _price; }
	std::string & TemplateFile () { return _templateFile; }
	void SetType (Type type) { _type = type; }
	Type GetType () const { return _type; }
	bool & Preview () { return _preview; }
private:
	std::string _comment;
	std::string _email;
	std::string	_licensee;
	int			_licCount;
	int			_startNum;
	int			_seatCount;
	char		_product;
	int			_bcSeatCount;
	int			_price;
	std::string	_templateFile;
	Type		_type;
	bool		_preview;
};

#endif
