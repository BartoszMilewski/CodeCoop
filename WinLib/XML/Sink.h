#if !defined (SINK_H)
#define SINK_H

namespace XML
{
	class Error
	{
	public:
		enum ErrorCode
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
		Error (ErrorCode errorCode);
		char const * GetMsg () { return _errorMsg [_err]; }
	private:
		ErrorCode _err;
		static char const * _errorMsg [];
	};

	class Sink
	{
	public:
		virtual ~Sink () {}
		virtual void OnStartTag (std::string const & tagName) {}
		virtual void OnEndTag () {}
		virtual void OnEndTagEmpty () {}
		virtual void OnAttribute (std::string const & attrib, std::string const & value) {}
		virtual void OnText (std::string const & text) {}
		virtual void OnComment (std::string const & comment) {}
		virtual void OnError (XML::Error const & error) {}
	};
}

#endif
