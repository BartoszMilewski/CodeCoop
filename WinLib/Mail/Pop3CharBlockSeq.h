#if !defined (POP3CHARBLOCKSEQ_H)
#define POP3CHARBLOCKSEQ_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

#include <Parse/BufferedStream.h>
#include <Mail/Sink.h>

namespace Pop3
{
	// stops at first empty line  or 
	// at one of the following special characters: " ", '-', '.' found at the beginning of a line
	// a client must not call Get after a call to Get returned '\0' 
	class CharBlockSeq : public GenericInput<'\0'>
	{
	public:
		CharBlockSeq (LineSeq & lineSeq)
			: _lineSeq (lineSeq),
			_currentCharIdx (0)
		{
			Assert (!_lineSeq.AtEnd ());
			Assert (!_lineSeq.Get ().empty ());
			_currentLine = &_lineSeq.Get ();
		}
		char Get ()
		{ 
			if (_currentCharIdx == _currentLine->size ())
				return NextLine ();

			return (*_currentLine) [_currentCharIdx++];
		}
	private:
		char NextLine ()
		{
			Assert (!_lineSeq.AtEnd ());

			_lineSeq.Advance ();
			if (_lineSeq.AtEnd ())
				return '\0';

			_currentLine = &_lineSeq.Get ();
			if (_currentLine->size () == 0)
				return '\0';

			char c = (*_currentLine) [0];

			if (c == '-' || c == ' ' || c == '.')
				return '\0';

			_currentCharIdx = 1;
			return c;
		}
	private:
		LineSeq			  & _lineSeq;
		std::string const *	_currentLine;
		unsigned int		_currentCharIdx;
	};
}

#endif
