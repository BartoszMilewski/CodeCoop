#if !defined (SCANNER_H)
#define SCANNER_H
// ---------------------------
// (c) Reliable Software, 2003
// ---------------------------
#include <iostream>

namespace XML
{
	class Scanner
	{
	public:
		enum Token
		{
			Unrecognized,
			XMLTag,		// <?
			DocType,    // <!DOCTYPE ... >
			StartTagBra,// <tag
			EndTagBra,	// </tag
			EmptyTagKet,// />
			TagKet,	    // >
			Comment,	// <!--
			Attribute,	// attrib="value"
			Text,
			End			// EOF
		};
	public:
		Scanner (std::istream & in);
		void Accept ();
		Token GetToken () const { return _token; }
		void SkipXmlTag ();
		void SkipDocType ();

		std::string const & GetText () const { return _text; }
		std::string const & GetTagName () const { return _text; }
		std::string const & GetAttribName () const { return _text; }
		std::string const & GetArrtibValue () const { return _value; }

	private:
		void EatWhite ();
		void AcceptAttribute ();
		void AcceptText ();
		void AcceptComment ();

		void ReadValue ();
		void ReadWord ();
		void ReadComment ();
		void Read (std::string & buf, int len);

		static bool IsCommentStart (std::string const & str);
		static bool IsDocType (std::string const & str);
	private:
		std::istream & _in;
		Token		   _token;
		std::string	   _text;
		std::string	   _value;
		int			   _look;
		// state
		bool		   _inTag;
		bool		   _inComment;
	};
}

#endif
