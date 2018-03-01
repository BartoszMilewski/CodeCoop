// ----------------------------------
// (c) Reliable Software, 2003 - 2004
// ----------------------------------
#include "WinLibBase.h"
#include "XmlParser.h"
#include "Scanner.h"
#include "Sink.h"

namespace XML
{
	Parser::Parser (Scanner & scanner, Sink & sink)
		: _scanner (scanner),
		  _sink (sink)
	{}

	void Parser::Parse ()
	{
		// "A textual object is a well-defined XML document if
		// "taken as a whole it matches the "Document" production"
		Document ();
	}

	// Document	::=	Prolog Element Misc*
	void Parser::Document ()
	{
		Prolog ();
		Element ();
		Misc ();

		if (_scanner.GetToken () != Scanner::End)
		{
			_sink.OnError (Error::ExpectedEndOfFile);
		}
	}

	// Prolog ::= XMLDecl Misc* (DocTypeDecl Misc*)?
	void Parser::Prolog ()
	{
		XMLDecl ();
		Misc ();

		if (_scanner.GetToken () == Scanner::DocType)
		{
			DocTypeDecl ();
		}
		Misc ();
	}

	// XMLDecl ::= '<?xml' ... ?>'
	void Parser::XMLDecl ()
	{
		// Revisit: we don't have XML Declaration in CoopVersion.xml
		if (_scanner.GetToken () == Scanner::XMLTag)
		{
			_scanner.SkipXmlTag ();
			_scanner.Accept ();
		}
		else
		{
			// _sink.OnError (Error::ExpectedXMLDecl);
		}
	}

	// DocTypeDecl ::= '<!DOCTYPE' ... '>'
	void Parser::DocTypeDecl ()
	{
		// out of concern
		_scanner.SkipDocType ();
		_scanner.Accept ();
	}

	void Parser::Misc ()
	{
		while (_scanner.GetToken () == Scanner::Comment)
		{
			Comment ();
		}
	}

	// Element ::= EmptyElemTag | StartTag Content EndTag
	void Parser::Element ()
	{
		if (StartTag ()) // has body?
		{
			Content ();
			EndTag ();
		}
	}

	// Content ::= CharData? ((Element | Comment ...) CharData?)*
	void Parser::Content ()
	{
		CharData ();

		while (_scanner.GetToken () == Scanner::Comment ||
			   _scanner.GetToken () == Scanner::StartTagBra ||
			   _scanner.GetToken () == Scanner::Text)
		{
			if (_scanner.GetToken () == Scanner::StartTagBra)
				Element ();
			else if (_scanner.GetToken () == Scanner::Comment)
				Comment ();
			else
				CharData ();
		}
	}

	// StartTag ::= StartTagBra, ElementName, {Attribute}, TagKet || EmptyTagKet
	// Returns true, if is not empty -- <tag [{attrib='value'}]/>
	bool Parser::StartTag ()
	{
		if (_scanner.GetToken () != Scanner::StartTagBra)
		{
			_sink.OnError (Error::ExpectedStartTag);
			return false;
		}

		ElementName ();
		while (_scanner.GetToken () == Scanner::Attribute)
		{
			Attribute ();
		}
		if (_scanner.GetToken () == Scanner::TagKet)
		{
			_scanner.Accept (); // >
		}
		else if (_scanner.GetToken () == Scanner::EmptyTagKet)
		{
			_scanner.Accept (); // />
			_sink.OnEndTagEmpty ();
			return false;
		}
		else
		{
			_sink.OnError (Error::ExpectedTagKet);
			return false;
		}

		return true;
	}

	// ElementName ::= case sensitive string without spaces
	void Parser::ElementName ()
	{
		_sink.OnStartTag (_scanner.GetTagName ());
		_scanner.Accept ();
	}

	// Attribute ::= AttributeName="AttributeValue"
	void Parser::Attribute ()
	{
		Assert (_scanner.GetToken () == Scanner::Attribute);
		_sink.OnAttribute (_scanner.GetAttribName (), _scanner.GetArrtibValue ());
		_scanner.Accept ();
	}

	// EndTag ::= EndTagBra, ElementName, TagKet
	void Parser::EndTag ()
	{
		if (_scanner.GetToken () == Scanner::EndTagBra)
		{
			// Revisit: check whether elementName is the same as in StartTag
			// If not, we've got a syntax error. Tell the sink to invalidate 
			// the element together with its contents
			_sink.OnEndTag ();
			_scanner.Accept ();
			if (_scanner.GetToken () == Scanner::TagKet)
			{
				_scanner.Accept (); // >
			}
			else
				_sink.OnError (Error::ExpectedTagKet);
		}
		else
			_sink.OnError (Error::ExpectedEndTag);
	}

	// Comment ::= string
	void Parser::Comment ()
	{
		_sink.OnComment (_scanner.GetText ());
		_scanner.Accept ();
	}

	// CharData ::=	general string ...
	void Parser::CharData ()
	{
		if (_scanner.GetToken () == Scanner::Text)
		{
			_sink.OnText (_scanner.GetText ());
			_scanner.Accept ();
		}
	}
}
