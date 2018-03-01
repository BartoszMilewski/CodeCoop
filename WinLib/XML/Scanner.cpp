// ---------------------------
// (c) Reliable Software, 2003
// ---------------------------
#include <WinLibBase.h>
#include "Scanner.h"
#include <StringOp.h>

namespace XML
{
	Scanner::Scanner (std::istream & in)
		: _in (in), _inTag (false), _inComment (false)
	{
		_look = in.get ();
		Accept ();
	}

	void Scanner::Accept ()
	{
		_token = Unrecognized;

		if (_inComment)
		{
			AcceptComment ();
		}

		EatWhite ();

		if (_look == EOF || _look == '\0')
		{
			_token = End;
			return;
		}

		if (_inTag)
		{
			if (_look == '/' && _in.peek () == '>')
			{
				_token = EmptyTagKet;
				_inTag = false;
				_look = _in.get ();
				_look = _in.get ();
			}
			else if (_look == '>')
			{
				_token = TagKet;
				_inTag = false;
				_look = _in.get ();
			}
			else
			{
				_token = Attribute;
				AcceptAttribute ();
			}
		}
		else if (_look == '<')
		{
			_text.clear ();
			_look = _in.get ();
			if (_look == '/')
			{
				_inTag = true;
				_token = EndTagBra;
				_look = _in.get ();
				ReadWord ();
			}
			else if (_look == '?')
			{
				_token = XMLTag;
				_look = _in.get ();
			}
			else if (_look == '!')
			{
				// comment or doctypedecl
				std::string tmp;
				Read (tmp, 3);
				if (IsCommentStart (tmp))
				{
					_token = Comment;
					_inComment = true;
					ReadComment ();
				}
				else
				{
					Read (tmp, 5);
                    if (IsDocType (tmp))
					{
						_token = DocType;
						_look = _in.get ();
					}
					else
					{
						_text += tmp;
					}
				}
			}
			
			if (_token == Unrecognized)
			{
				_inTag = true;
				_token = StartTagBra;
				ReadWord ();
			}
		}
		else if (_look == '>')
		{
			_token = TagKet;
			_inTag = false;
			_look = _in.get ();
		}
		else
			AcceptText ();
	}

	bool Scanner::IsCommentStart (std::string const & str)
	{
		return str == std::string ("!--");
	}

	bool Scanner::IsDocType (std::string const & str)
	{
		return IsNocaseEqual (str, "!DOCTYPE");
	}

	void Scanner::ReadWord ()
	{
		do
		{
			_text += _look;
			_look = _in.get ();
		} while (!isspace (_look) && _look != '>' && _look != '/');
	}

	void Scanner::ReadComment ()
	{
		int len = 0;
		do
		{
			do
			{
				_text += _look;
				_look = _in.get ();
			} while (_look != EOF && _look != '>');
			len = _text.length ();
		} while (_look != EOF && (len < 2 
			|| _text [len - 1] != '-' || _text [len - 2] != '-'));

		_text.resize (len - 2); // remove trailing --
	}

	void Scanner::AcceptComment ()
	{
		Assert (_look == '>');
		_inComment = false;
		_look = _in.get ();
	}

	void Scanner::AcceptText ()
	{
		Assert (_look != EOF && _look != '<');
		_token = Text;
		_text.clear ();
		do
		{
			if (isspace (_look))
				_text += ' ';
			else
			{
				_text += _look;
			}
			_look = _in.get ();
		} while (_look != EOF && _look != '<');
	}

	void Scanner::AcceptAttribute ()
	{
		_text.clear ();
		_value.clear ();
		while (_look != '>' && _look != '=' && !isspace (_look))
		{
			_text += _look;
			_look = _in.get ();
		}
		// EatWhite ?
		if (_look == '=')
		{
			// EatWhite ?
			_look = _in.get ();
			if (_look == '"')
				ReadValue ();
		}
	}

	void Scanner::ReadValue ()
	{
		_look = _in.get ();
		while (_look != EOF && _look != '"' && _look != '>')
		{
			_value += _look;
			_look = _in.get ();
		}
		if (_look == '"')
			_look = _in.get ();
	}

	void Scanner::EatWhite ()
	{
		while (isspace (_look))
			_look = _in.get ();
	}

	void Scanner::SkipXmlTag ()
	{
		while (_look != EOF)
		{
			if (_look == '?' && _in.peek () == '>')
			{
				_look = _in.get ();
				_look = _in.get ();
				break;
			}
			_look = _in.get ();
		}
	}
	
	void Scanner::SkipDocType ()
	{
		while (_look != EOF)
		{
			if (_look == '>')
			{
				_look = _in.get ();
				break;
			}
			_look = _in.get ();
		}
	}

	void Scanner::Read (std::string & buf, int len)
	{
		while (_look != EOF && len > 0)
		{
			buf += _look;
			_look = _in.get ();
			--len;
		}
	}
}
