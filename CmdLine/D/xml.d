module xml;
import std.stream;
import std.string;
import std.ctype;

class Error
{
public:
	enum Code
	{
		// Revisit: more codes
		ExpectedEndOfFile = 0,
		ExpectedXMLDecl,
		ExpectedStartTag,
		ExpectedEndTag,
		ExpectedTagKet,
		Count
	};
public:
	static this ()
	{
		_errorMsg.length = Code.Count;
		_errorMsg [Code.ExpectedEndOfFile] = "Expected end of file";
		_errorMsg [Code.ExpectedXMLDecl] = "Expected XML declaration tag";
		_errorMsg [Code.ExpectedStartTag] = "Expected start of tag";
		_errorMsg [Code.ExpectedEndTag] = "Expected end of tag";
		_errorMsg [Code.ExpectedTagKet] = "End tag not closed";
	}
	this (Code errorCode)
	{
		_err = errorCode;
	}
	string GetMsg () { return _errorMsg [_err]; }
private:
	Code _err;
	static string _errorMsg [];
}

interface Sink
{
public:
	void OnStartTag (string tagName);
	void OnEndTag ();
	void OnEndSoloTag ();
	void OnAttribute (string name, string value);
	void OnText (string text);
	void OnComment (string text);
	void OnError (Error.Code err);
}

class Attribute
{
public:
	this (string name, string value) 
	{
		_name = name;
		_value = value;
	}
	string Name () { return _name; }
	string Value () { return _value; }

	void TransformValue (string value)
	{
		// TBD
	}
	string TransformValue ()
	{
		// TBD
		return _value;
	}
private:
	string _name;
	string _value;
}

class Node
{
public:
	this (string name, bool closed = false)
	{
		_name = name;
		_closed = closed;
	}
	string Name () { return _name; }
	void Name (string name) { _name = name; }
	Attribute [] Attributes () { return _attributes; }
	bool FindAttribute (string name, ref string value)
	{
		foreach (attr; _attributes)
		{
			if (attr.Name == name)
			{
				value = attr.Value;
				return true;
			}
		}
		return false; 
	}
	Node [] Children () { return _children; }
	void AddChild (Node child) { _children ~= child; }
	void AddAttribute (Attribute attr) { _attributes ~= attr; }
	
	int opApply(int delegate(ref Node) dg)
	{
		int result;
		foreach (child; _children)
		{
			result = dg (child);
			if (result)
				break;
		}
		return result;
	}
private:
	string _name;
	Attribute [] _attributes;
	Node [] _children;
	bool _closed;
}

class Tree
{
public:
	this () { _top = new Node ("Top"); }
	void Root (Node root) { _top.AddChild (root); }
	Node Root ()
	{
		if (_top.Children.length != 0)
			return _top.Children [0];
		else
			return null;
	}
private:
	Node _top;
}

class TreeMaker : Sink
{
public:
	this (Tree tree)
	{
		_curr = tree._top;
	}
	void OnStartTag (string tagName)
	{
		Node newChild = new Node (tagName);
		_curr.AddChild (newChild);
		_parents ~= _curr;
		_curr = newChild;
	}
	void OnEndTag ()
	{
		assert (_parents.length != 0);
		_curr = _parents [$ - 1];
		_parents.length = _parents.length - 1;
	}
	void OnEndSoloTag () { OnEndTag (); }
	void OnAttribute (string name, string value)
	{
		assert (_curr !is null);
		Attribute attr = new Attribute (name, value);
		_curr.AddAttribute (attr);
	}
	void OnText (string text)
	{
		assert (_curr !is null);
		Node child = new Node ("");
		_curr.AddChild (child);
		Attribute attr = new Attribute ("Text", text);
		child.AddAttribute (attr);
	}
	void OnComment (string text)
	{
	}
	void OnError (Error.Code err)
	{
		Error e = new Error (err);
		throw new Exception (e.GetMsg);
	}
private:
	Node _curr;
	Node [] _parents;
}

class Scanner
{
private:
	final char Peek ()
	{
		char c = _in.getc;
		if (c != char.init)
			_in.ungetc (c);
		return c;
	}
public:
	// Start/end tags: <a attributes> text </a>
	// Solo tag:       <img attributes/> <br/>
	enum Token
	{
		Unrecognized,
		XMLTag,		// <?
		DocType,    // <!DOCTYPE ... >
		StartTagBra,// <tag
		EndTagBra,	// </tag
		SoloTagKet, // />
		TagKet,	    // >
		Comment,	// <!--
		Attribute,	// attrib="value"
		Text,
		End			// EOF
	}

	this (InputStream inp)
	{
		_in = inp;
		_lookAhead = _in.getc;
		Accept ();
	}
	
	void Accept ()
	{
		_token = Token.Unrecognized;
		if (_inComment)
			AcceptComment ();
			
		EatWhite ();
		
		if (_lookAhead == char.init || _lookAhead == '\0')
		{
			_token = Token.End;
			return;
		}
		
		if (_inTag)
		{
			if (_lookAhead == '/' && Peek () == '>')
			{
				_token = Token.SoloTagKet;
				_inTag = false;
				_lookAhead = _in.getc;
				_lookAhead = _in.getc;
			}
			else if (_lookAhead == '>')
			{
				_token = Token.TagKet;
				_inTag = false;
				_lookAhead = _in.getc;
			}
			else
			{
				_token = Token.Attribute;
				AcceptAttribute ();
			}
		}
		else if (_lookAhead == '<')
		{
			_text.length = 0;
			_lookAhead = _in.getc;
			if (_lookAhead == '/')
			{
				_inTag = true;
				_token = Token.EndTagBra;
				_lookAhead = _in.getc;
				ReadWord ();
			}
			else if (_lookAhead == '?')
			{
				_token = Token.XMLTag;
				_lookAhead = _in.getc;
			}
			else if (_lookAhead == '!')
			{
				// comment or doctypedecl
				string tmp;
				Read (tmp, 3);
				if (IsCommentStart (tmp))
				{
					_token = Token.Comment;
					_inComment = true;
					ReadComment ();
				}
				else
				{
					Read (tmp, 5);
                    if (IsDocType (tmp))
					{
						_token = Token.DocType;
						_lookAhead = _in.getc;
					}
					else
					{
						_text = _text ~ tmp;
					}
				}
			}
			
			if (_token == Token.Unrecognized)
			{
				_inTag = true;
				_token = Token.StartTagBra;
				ReadWord ();
			}
		}
		else if (_lookAhead == '>')
		{
			_token = Token.TagKet;
			_inTag = false;
			_lookAhead = _in.getc;
		}
		else
			AcceptText ();
	}
	Token GetToken () const { return _token; }
	void SkipXmlTag ()
	{
		while (_lookAhead != char.init)
		{
			if (_lookAhead == '?' && Peek () == '>')
			{
				_lookAhead = _in.getc;
				_lookAhead = _in.getc;
				break;
			}
			_lookAhead = _in.getc;
		}
	}
	void SkipDocType ()
	{
		while (_lookAhead != char.init)
		{
			if (_lookAhead == '>')
			{
				_lookAhead = _in.getc;
				break;
			}
			_lookAhead = _in.getc;
		}
	}
	
	string Text () { return _text; }
	string TagName () { return _text; }
	string AttributeName () { return _text; }
	string AttributeValue () { return _value; }
private:
	void EatWhite ()
	{
		while (isspace (_lookAhead))
			_lookAhead = _in.getc;
	}
	void AcceptAttribute ()
	{
		_text.length = 0;
		_value.length = 0;
		while (_lookAhead != '>' && _lookAhead != '=' && !isspace (_lookAhead))
		{
			_text = _text ~ _lookAhead;
			_lookAhead = _in.getc;
		}
		// EatWhite ?
		if (_lookAhead == '=')
		{
			// EatWhite ?
			_lookAhead = _in.getc;
			if (_lookAhead == '"')
				ReadValue ();
		}
	}
	void AcceptText ()
	{
		assert (_lookAhead != char.init && _lookAhead != '<');
		_token = Token.Text;
		_text.length = 0;
		do
		{
			if (isspace (_lookAhead))
				_text = _text ~ ' ';
			else
			{
				_text = _text ~ _lookAhead;
			}
			_lookAhead = _in.getc;
		} while (_lookAhead != char.init && _lookAhead != '<');
	}
	void AcceptComment ()
	{
		assert (_lookAhead == '>');
		_inComment = false;
		_lookAhead = _in.getc;
	}

	void ReadValue ()
	{
		_lookAhead = _in.getc;
		while (_lookAhead != char.init && _lookAhead != '"' && _lookAhead != '>')
		{
			_value = _value ~ _lookAhead;
			_lookAhead = _in.getc;
		}
		if (_lookAhead == '"')
			_lookAhead = _in.getc;
	}
	void ReadWord ()
	{
		do
		{
			_text = _text ~ _lookAhead;
			_lookAhead = _in.getc;
		} while (!isspace (_lookAhead) && _lookAhead != '>');
	}
	void ReadComment ()
	{
		int len = 0;
		do
		{
			do
			{
				_text = _text ~ _lookAhead;
				_lookAhead = _in.getc;
			} while (_lookAhead != char.init && _lookAhead != '>');
			len = _text.length;
		} while (_lookAhead != char.init && (len < 2 
			|| _text [len - 1] != '-' || _text [len - 2] != '-'));

		_text.length = len - 2; // remove trailing --
	}
	void Read (string buf, int len)
	{
		while (_lookAhead != char.init && len > 0)
		{
			buf = buf ~ _lookAhead;
			_lookAhead = _in.getc;
			--len;
		}
	}

	static bool IsCommentStart (string str)
	{
		return str == "!--";
	}
	static bool IsDocType (string str)
	{
		//return IsNocaseEqual (str, "!DOCTYPE");
		return str == "!DOCTYPE";
	}

private:
	InputStream _in;
	Token		_token;
	string		_text;
	string		_value;
	char		_lookAhead;
	// state
	bool		_inTag;
	bool		_inComment;
}

class Parser
{
public:
	this (Scanner scanner, Sink sink)
	{
		_scanner = scanner;
		_sink = sink;
	}
	void Parse ()
	{
		// "A textual object is a well-defined XML document if
		// "taken as a whole it matches the "Document" production"
		Document ();
	}
private:
	// Document	::=	Prolog Element Misc*
	void Document ()
	{
		Prolog ();
		Element ();
		Misc ();

		if (_scanner.GetToken () != Scanner.Token.End)
		{
			_sink.OnError (Error.Code.ExpectedEndOfFile);
		}
	}
	// Prolog ::= XMLDecl Misc* (DocTypeDecl Misc*)?
	void Prolog ()
	{
		XMLDecl ();
		Misc ();

		if (_scanner.GetToken () == Scanner.Token.DocType)
		{
			DocTypeDecl ();
		}
		Misc ();
	}

	// XMLDecl ::= '<?xml' ... ?>'
	void XMLDecl ()
	{
		if (_scanner.GetToken () == Scanner.Token.XMLTag)
		{
			_scanner.SkipXmlTag ();
			_scanner.Accept ();
		}
		else
		{
			// _sink.OnError (Error.Code.ExpectedXMLDecl);
		}
	}

	// DocTypeDecl ::= '<!DOCTYPE' ... '>'
	void DocTypeDecl ()
	{
		// we ignore it for now
		_scanner.SkipDocType ();
		_scanner.Accept ();
	}
	
	void Misc ()
	{
		while (_scanner.GetToken () == Scanner.Token.Comment)
		{
			Comment ();
		}
	}

	// Element ::= SoloElemTag | StartTag Content EndTag
	void Element ()
	{
		if (StartTag ()) // has body?
		{
			Content ();
			EndTag ();
		}
	}

	// Content ::= CharData? ((Element | Comment ...) CharData?)*
	void Content ()
	{
		CharData ();

		while (_scanner.GetToken () == Scanner.Token.Comment ||
			   _scanner.GetToken () == Scanner.Token.StartTagBra ||
			   _scanner.GetToken () == Scanner.Token.Text)
		{
			if (_scanner.GetToken () == Scanner.Token.StartTagBra)
				Element ();
			else if (_scanner.GetToken () == Scanner.Token.Comment)
				Comment ();
			else
				CharData ();
		}
	}

	// StartTag ::= StartTagBra, ElementName, {Attribute}, TagKet | SoloTagKet
	// Returns true, if is not solo -- <tag [{attrib='value'}]/>
	bool StartTag ()
	{
		if (_scanner.GetToken () != Scanner.Token.StartTagBra)
		{
			_sink.OnError (Error.Code.ExpectedStartTag);
			return false;
		}

		ElementName ();
		while (_scanner.GetToken () == Scanner.Token.Attribute)
		{
			Attribute ();
		}

		if (_scanner.GetToken () == Scanner.Token.TagKet)
		{
			_scanner.Accept (); // >
			return true;
		}
		else if (_scanner.GetToken () == Scanner.Token.SoloTagKet)
		{
			_scanner.Accept (); // />
			_sink.OnEndSoloTag ();
			return false;
		}
		else
		{
			_sink.OnError (Error.Code.ExpectedTagKet);
			return false;
		}

		return true;
	}

	// EndTag ::= EndTagBra, ElementName, TagKet
	void EndTag ()
	{
		if (_scanner.GetToken () == Scanner.Token.EndTagBra)
		{
			// Revisit: check whether elementName is the same as in StartTag
			// If not, we've got a syntax error. Tell the sink to invalidate 
			// the element together with its contents
			_sink.OnEndTag ();
			_scanner.Accept ();
			if (_scanner.GetToken () == Scanner.Token.TagKet)
			{
				_scanner.Accept (); // >
			}
			else
				_sink.OnError (Error.Code.ExpectedTagKet);
		}
		else
			_sink.OnError (Error.Code.ExpectedEndTag);
	}

	// ElementName ::= case sensitive string without spaces
	void ElementName ()
	{
		_sink.OnStartTag (_scanner.TagName);
		_scanner.Accept ();
	}

	// Attribute ::= AttributeName="AttributeValue"
	void Attribute ()
	{
		assert (_scanner.GetToken () == Scanner.Token.Attribute);
		_sink.OnAttribute (_scanner.AttributeName, _scanner.AttributeValue);
		_scanner.Accept ();
	}

	// Comment ::= string
	void Comment ()
	{
		_sink.OnComment (_scanner.Text);
		_scanner.Accept ();
	}

	// CharData ::=	general string ...
	void CharData ()
	{
		if (_scanner.GetToken () == Scanner.Token.Text)
		{
			_sink.OnText (_scanner.Text);
			_scanner.Accept ();
		}
	}
private:
	Scanner _scanner;
	Sink	_sink;
}

unittest
{
	Tree tree = new Tree;
	TreeMaker tm = new TreeMaker (tree);
	tm.OnStartTag ("Parent");
	tm.OnStartTag ("Child");
	tm.OnAttribute ("color", "blue");
	tm.OnAttribute ("color", "red");
	tm.OnEndTag ();
	tm.OnStartTag ("Child");
	tm.OnAttribute ("color", "yellow");
	tm.OnAttribute ("color", "green");
	tm.OnEndTag ();
	tm.OnEndTag ();
	
	auto root = tree.Root ();
	assert (root.Name == "Parent");
	assert (root.Children.length == 2);
	foreach (n; root.Children)
	{
		assert (n.Name == "Child");
		assert (n.Attributes.length == 2);
		foreach (a; n.Attributes)
		{
			assert (a.Name == "color");
			writefln ("color = %s", a.Value);
		}
	}
}
