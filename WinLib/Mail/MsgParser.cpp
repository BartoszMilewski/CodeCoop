// ----------------------------------
// (c) Reliable Software, 2005 - 2006
// ----------------------------------

#include "WinLibBase.h"
#include "MsgParser.h"
#include "HeaderSeq.h"
#include "Sink.h"
#include "Pop3.h"
#include "Pop3CharBlockSeq.h"
#include <Parse/BufferedStream.h>

void GetBoundary (MIME::Headers const & context, std::string & boundary)
{
	boundary.clear ();
	MIME::Headers::Attributes const & typeAttr = context.GetTypeAttributes ();
	MIME::Headers::Attributes::const_iterator result = typeAttr.find ("boundary");
	if (result == typeAttr.end ())
		throw Pop3::MsgCorruptException ("POP3: Corrupted message headers. No boundary defined.");
	boundary = result->second;
	if (boundary.empty ())
		throw Pop3::MsgCorruptException ("POP3: Corrupted message headers. Illegal boundary defined.");
	// prepend boundary with --
	boundary.insert (0, "--");
}

void Pop3::Parser::Parse (LineSeq & lineSeq, Pop3::Sink & sink)
{
	_lineSeq = &lineSeq;
	_sink    = &sink;

	while (!_context.empty ())
		_context.pop ();
	_currentContext.Clear ();

	Message ();
	Assert (_lineSeq->AtEnd ());
}

void Pop3::Parser::Message ()
{
	Headers ();
	Body ();
}

void Pop3::Parser::Headers ()
{
	_sink->OnHeadersStart ();
	for (HeaderSeq hdr (*_lineSeq); !hdr.AtEnd (); hdr.Advance ())
	{
		// remember MIME headers -- they create parser context
		if (hdr.IsName ("Content-Type"))
		{
			_currentContext.SetType (hdr.GetValue (), hdr.GetComment ());
		}
		else if (hdr.IsName ("Content-Transfer-Encoding"))
		{
			_currentContext.SetEncoding (hdr.GetValue ());
		}
		else if (hdr.IsName ("Content-Disposition"))
		{
			_currentContext.SetDisposition (hdr.GetValue (), hdr.GetComment ());
		}
		_sink->OnHeader (hdr.GetName (), hdr.GetValue (), hdr.GetComment ());
	}


	// omit the obligatory empty line separating headers from body
	// and any empty lines following headers
	while (!_lineSeq->AtEnd () && _lineSeq->Get ().empty ()) 
	{	
		_lineSeq->Advance ();
	}

	_sink->OnHeadersEnd ();
}

void Pop3::Parser::Body ()
{
	if (_lineSeq->AtEnd ())
		return;

	_sink->OnBodyStart ();

	if (_currentContext.IsMultiPart ())
	{
		MultiPart ();
	}
	else
	{
		SimplePart ();
	}

	if (_context.empty ()) // root level
	{
		// RFC #2046: implementers must ignore anything that appears 
		// after the last boundary delimeter line
		// or anything that appears after a simple part
		EatToEnd ();
	}

	_sink->OnBodyEnd ();
}

// Recursive
void Pop3::Parser::MultiPart ()
{
	std::string boundary;
	GetBoundary (_currentContext, boundary);
	unsigned int boundaryLen = boundary.size ();
	while (!_lineSeq->AtEnd ())
	{
		// Not a MIME boundary
		// RFC #2046: implementers must ignore "preamble".
		// Still we cannot just skip this line if we want to 
		// be able to serialize the whole original message
		if (!EatToLine (boundary))
			throw Pop3::MsgCorruptException ("POP3: Corrupted message syntax. "
										  "Boundary not found in multipart message.");
		Assert (!_lineSeq->AtEnd ());
		std::string const line = _lineSeq->Get ();
		unsigned lineLen = line.size ();
		Assert (line.compare (0, boundaryLen, boundary) == 0);
		if (lineLen >= boundaryLen + 2 && line [boundaryLen] == '-' && line [boundaryLen + 1] == '-')
		{
			// Closing boundary
			_lineSeq->Advance ();
			break;
		}
		else
		{
			_lineSeq->Advance ();
			_context.push (_currentContext);
			_currentContext.Clear ();
			// Recurse
			Message ();
			_currentContext = _context.top ();
			_context.pop ();
		}
	}
}

void Pop3::Parser::SimplePart ()
{
	if (_currentContext.IsApplication () && // type		:	application
		_currentContext.IsOctetStream () &&	// subtype  :	octet-stream
		_currentContext.IsBase64 ())	    // encoding :	base64
	{
		AppOctetStreamBase64Part ();
	}
	else if (_currentContext.IsPlainText ())
	{
		if (_context.size () > 0) // multipart message
			PlainTextPart ();
		else
			PlainTextMessage ();
	}
	else
	{
		// Revisit: handle other simple parts
		if (_context.size () > 0) // multipart message
		{
			std::string boundary;
			GetBoundary (_context.top (), boundary);
			if (!EatToLine (boundary))
				throw Pop3::MsgCorruptException ("POP3: Corrupted message syntax. "
											  "Boundary not found in multipart message.");
		}
		else
		{
			EatToEnd ();
		}
	}
}

void Pop3::Parser::AppOctetStreamBase64Part ()
{
	MIME::Headers::Attributes const & dispositionAttr = _currentContext.GetDispositionAttributes ();
	MIME::Headers::Attributes::const_iterator result = dispositionAttr.find ("filename");
	std::string attFilename;
	if (result == dispositionAttr.end ())
	{
		MIME::Headers::Attributes const & typeAttr = _currentContext.GetTypeAttributes ();
		MIME::Headers::Attributes::const_iterator result2 = typeAttr.find ("name");
		if (result2 == typeAttr.end ())
		{
			throw Pop3::MsgCorruptException ("POP3: Unrecognized message syntax. "
				"No name defined for an attachment.");
		}
		else
		{
			attFilename = result2->second;
		}
	}
	else
	{
		attFilename = result->second;
	}
	if (attFilename.empty ())
		throw Pop3::MsgCorruptException ("POP3: Unrecognized message syntax. "
			"Attachment name cannot be empty.");
	if (_lineSeq->AtEnd ())
		throw Pop3::MsgCorruptException ("POP3: Corrupted message attachment.");

	Pop3::CharBlockSeq charSeq (*_lineSeq);
	_sink->OnAttachment (charSeq, attFilename);
}

void Pop3::Parser::PlainTextPart ()
{
	Assert (_context.size () > 0);
	std::string boundary;
	GetBoundary (_context.top (), boundary);
	unsigned int const boundaryLen = boundary.size ();	
	bool isBoundaryFound = false;
	std::string text;
	while (!_lineSeq->AtEnd ())
	{
		std::string const line = _lineSeq->Get ();
		if (line.compare (0, boundaryLen, boundary) == 0)
		{
			isBoundaryFound = true;
			break;
		}

		text += line;
		text += '\n';

		_lineSeq->Advance ();
	};

	if (!isBoundaryFound)
		throw Pop3::MsgCorruptException ("POP3: Invalid plain text section. No boundary found.");
	
	if (!text.empty ())
		text.resize (text.size () - 1); // remove '\n' at the end

	_sink->OnText (text);
}

void Pop3::Parser::PlainTextMessage ()
{
	std::string text;
	while (!_lineSeq->AtEnd ())
	{
		text += _lineSeq->Get ();
		text += '\n';
		_lineSeq->Advance ();
	};
	
	if (!text.empty ())
		text.resize (text.size () - 1); // remove '\n' at the end
	
	_sink->OnText (text);
}

void Pop3::Parser::EatToEnd ()
{
	while (!_lineSeq->AtEnd ())
		_lineSeq->Advance ();
}

bool Pop3::Parser::EatToLine (std::string const & stopLine)
{
	unsigned int stopLineLen = stopLine.size ();
	while (!_lineSeq->AtEnd ())
	{
		std::string const & currentLine = _lineSeq->Get ();
		if (currentLine.compare (0, stopLineLen, stopLine) == 0)
			return true;

		_lineSeq->Advance ();
	}
	return false;
}
