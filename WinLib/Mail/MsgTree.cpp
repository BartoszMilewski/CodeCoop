// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

#include <WinLibBase.h>
#include "MsgTree.h"
#include <Net/Socket.h>
#include <Mail/SmtpUtils.h>
#include <File/MemFile.h>
#include <Mail/Base64.h>
#include <sstream>

class IsHeaderNameEqual
{
public:
	IsHeaderNameEqual (std::string const & name)
		: _name (name)
	{}
	bool operator () (Header const & hdr) const 
	{
		return _name == hdr.GetName (); 
	}
private:
	std::string const & _name;
};

Header const * MessagePart::FindHeader (std::string const & name) const
{
	std::vector<Header>::const_iterator it = 
		std::find_if (_headers.begin (), _headers.end (), IsHeaderNameEqual (name));
	
	if (it != _headers.end ())
		return &(*it); 

	return 0;
}

void MultipartMixedPart::Send (Socket & socket) const
{
	// generate boundary
	std::string boundary;
	do
	{
		// our format: "----=_NextPart_16HexDigits"
		boundary = "----=_NextPart_";
		std::ostringstream hexStr;
		hexStr << std::uppercase << std::hex << rand () << rand () << rand () << rand ();
		boundary += hexStr.str ();
		for (unsigned int i = 0; i < _parts.size (); ++i)
		{
			_parts [i]->AgreeOnBoundary (boundary);
			if (boundary.empty ())
				break;
		}
	}
	while (boundary.empty ());

	// now send
	socket.SendLine ("Content-Type: multipart/mixed;");
	std::string boundaryField = "\tboundary=\"";
	boundaryField += boundary + "\"";
	socket.SendLine (boundaryField);
	socket.SendLine ();

	for (unsigned int i = 0; i < _parts.size (); ++i)
	{
		socket.SendLine ("\r\n--" + boundary);
		_parts [i]->Send (socket);
	}
	// End this multipart part
	socket.SendLine ("\r\n--" + boundary + "--");
}

void PlainTextPart::AgreeOnBoundary (std::string & boundary) const
{
	std::string::size_type startPos = 0;
	do 
	{
		startPos = _text.find (boundary, startPos);
		if (startPos == std::string::npos)
			break;

		unsigned int boundarySize = boundary.size ();
		if (boundarySize > 70)
		{
			// boundary generation must start from the beginning
			boundary.clear ();
			return;
		}

		std::ostringstream hexDigit;
		hexDigit << std::uppercase << std::hex;
		if (startPos + boundarySize == _text.size ())
		{
			// add an arbitraty hex digit
			hexDigit << rand () % 16;
		}
		else
		{
			// add a hex digit different from the first character following boundary in _text
			unsigned int next = _text [startPos + boundarySize];
			next = next % 15 + 1;
			hexDigit << next;
		}

		boundary.append (hexDigit.str ());

	} while (true);
}

void PlainTextPart::Send (Socket & socket) const
{
	socket.SendLine ("Content-Type: text/plain;");
	socket.SendLine ("\tcharset=\"iso-8859-2\"");
	socket.SendLine ("Content-Transfer-Encoding: 7bit");

	socket.SendLine ();

	for (LineBreakingSeq lineSeq (_text); !lineSeq.AtEnd (); lineSeq.Advance ())
	{
		socket.SendLine (lineSeq.GetLine ());
	}
}

ApplicationOctetStreamPart::ApplicationOctetStreamPart (FilePath const & attPath)
: _attPath (attPath)
{
	Assert (FilePath::IsAbsolute (_attPath.ToString ()));
	PathSplitter splitter (_attPath.ToString ());
	Assert (splitter.HasFileName ());
	_filename = splitter.GetFileName ();
	_filename += splitter.GetExtension ();
}

void ApplicationOctetStreamPart::Send (Socket & socket) const
{
	socket.SendLine ("Content-Type: application/octet-stream;");
	socket.SendLine ("\tname=\"" + _filename + "\"");
	socket.SendLine ("Content-Transfer-Encoding: base64");
	socket.SendLine ("Content-Disposition: attachment;");
	socket.SendLine ("\tfilename=\"" + _filename + "\"");
	socket.SendLine ();

	MemFileReadOnly srcFile (_attPath.ToString ());
	Smtp::Output output (socket);
	Base64::Encode (srcFile.GetBuf (), srcFile.GetBufSize (), output);

	socket.SendLine ();
}
