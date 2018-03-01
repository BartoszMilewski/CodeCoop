#if !defined (SMTPUTILS_H)
#define SMTPUTILS_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

#include <GenericIo.h>

class Socket;

namespace Smtp
{
	// SMTP maximum line length: 76 characters
	static const unsigned int LineLength = 76;
	static const unsigned int LineLengthWithCRLF = LineLength + 2; // LineLength + 2: space for CR + LF

	class Output : public GenericOutput
	{
	public:
		Output (Socket & socket);
		void Put (char c);
		void Flush ();
	private:
		Socket &			_socket;
		char				_currentLine [LineLengthWithCRLF];
		unsigned int		_currLineLength;
	};
}

std::string ToBracketedString (std::string const & str);

class LineBreakingSeq
{
public:
	LineBreakingSeq (std::string const & text, unsigned int maxLineLength = Smtp::LineLength);
	bool AtEnd () const { return _isAtEnd; }
	void Advance ();
	std::string const & GetLine () const
	{
		Assert (!AtEnd ());
		return _currentLine;
	}
private:
	std::string	const & _text;
	unsigned int const  _maxLineLength;

	bool				_isAddSpace;
	unsigned int		_currentIdx;
	std::string		    _currentLine;
	bool				_isAtEnd;
};

#endif
