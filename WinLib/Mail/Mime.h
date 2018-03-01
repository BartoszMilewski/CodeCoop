#if !defined (MIME_H)
#define MIME_H
// --------------------------
// (c) Reliable Software 2005
// --------------------------
#include <StringOp.h>

namespace MIME
{
	// based on RFC 2045
	class Headers
	{
	/*  
		- MIME-Version
		- Content-Type header:
			Syntax: "Content-Type" ":" type "/" subtype
						*(";" parameter)
					parameter = name=value
					value may be quoted
			The type, subtype, and parameter names are not case sensitive.
			Parameter values are normally case sensitive, but sometimes
			are interpreted in a case-insensitive fashion, depending on the
			intended use. (For example, multipart boundaries are case-sensitive,
			but the "access-type" parameter for message/External-body is not
			case-sensitive.)
		- Content-Transfer-Encoding header:
			Syntax: "Content-Transfer-Encoding" ":" mechanism
					mechanism := "7bit" / "8bit" / "binary" /
					"quoted-printable" / "base64" /
					ietf-token / x-token
			Values are not case sensitive.
		- Content-ID (optional)
		- Content-Description (optional)
		- Content-Disposition (optional)

		All of the header fields are subject to the	general syntactic rules 
		for header fields specified in RFC 822.  In	particular, all of these 
		header fields except for Content-Disposition can include RFC 822 comments, 
		which have no semantic content and should be ignored during MIME processing.
	*/
	public:
		typedef NocaseMap<std::string> Attributes;
	public:
		void Clear ()
		{
			_type.clear ();
			_subtype.clear ();
			_typeAttrib.clear ();
			_encoding.clear ();
			_disposition.clear ();
			_dispositionAttrib.clear ();
		}
		void SetType (std::string const & type, std::string const & comment);
		void SetEncoding (std::string const & encoding) { _encoding = encoding; }
		void SetDisposition (std::string const & disposition, std::string const & comment);

		bool IsMultiPart () const { return IsNocaseEqual (_type, "multipart"); }
		bool IsPlainText () const 
		{ 
			return IsNocaseEqual (_type, "text") && 
				   IsNocaseEqual (_subtype, "plain"); 
		}
		bool IsApplication () const { return IsNocaseEqual (_type, "application"); }
		bool IsOctetStream () const { return IsNocaseEqual (_subtype, "octet-stream"); }

		bool IsBase64 () const { return IsNocaseEqual (_encoding, "base64"); }

		std::string const & GetType () const { return _type; }
		std::string const & GetSubtype () const { return _subtype; }
		std::string const & GetEncoding () const { return _encoding; }
		Attributes const & GetTypeAttributes () const { return _typeAttrib; }
		Attributes const & GetDispositionAttributes () const { return _dispositionAttrib; }
	private:
		void ParseComment (std::string const & str, MIME::Headers::Attributes & attrib);
	private:
		std::string _type;
		std::string _subtype;
		Attributes  _typeAttrib;  // Content-Type header attributes
		std::string _encoding;
		std::string _disposition;
		Attributes  _dispositionAttrib; // Content-Disposition header attributes
	};
}

#endif
