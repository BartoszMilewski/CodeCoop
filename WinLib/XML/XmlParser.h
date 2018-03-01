#if !defined (XMLPARSER_H)
#define XMLPARSER_H
// --------------------------------
// (c) Reliable Software, 2003-2004
// --------------------------------

namespace XML
{
	class Scanner;
	class Sink;


// WWW.W3C.ORG was the source of XML grammar


// Chosen symbols of EBNF notation:
// A* - zero or more occurences of A
// A+ - one or more occurences of A
// A? - A or nothing, optional A
// |  - logical OR
// (expression) - treat expression as unit
// ... - simplified or omitted in our implementation

// Document		::=		Prolog Element Misc*

// Prolog		::=		XMLDecl Misc* (DocTypeDecl Misc*)?
// XMLDecl      ::=		'<?xml' ... ?>'
// DocTypeDecl  ::=		'<!DOCTYPE' ... '>'
// Misc			::=		Comment ...

// Element		::=		EmptyElemTag | StartTag Content EndTag
// EmptyElemTag ::=		StartTagBra ElementName Attribute* EmptyTagKet
// StartTag     ::=		StartTagBra ElementName Attribute* TagKet
// Content		::=		CharData? ((Element | Comment ...) CharData?)*
// EndTag		::=		EndTagBra, ElementName, TagKet

// Attribute    ::=		AttributeName="AttributeValue"
// Comment		::=		StartComment, CharData?, EndComment
// StartComment ::=		StartTagBra !--
// EndComment   ::=		-- TagKet

// ElementName, 
// AttributeName ::=	case sensitive string without spaces
// CharData		 ::=	general string ...

// AttributeValue ::=	general string

// StartTagBra	::= <
// EndTagBra	::= </
// TagKet		::= >
// EmptyTagKet  ::= />

	class Parser
	{
	public:
		Parser (Scanner & scanner, Sink & sink);
		void Parse ();
	private:
		void Document ();
		void Prolog ();
		void XMLDecl ();
		void DocTypeDecl ();
		void Misc ();

		void Element ();
		bool StartTag ();
		void Content ();
		void EndTag ();

		void ElementName ();
		void Attribute ();
		void Comment ();
		void CharData ();

	private:
		Scanner & _scanner;
		Sink	& _sink;
	};
}

#endif
