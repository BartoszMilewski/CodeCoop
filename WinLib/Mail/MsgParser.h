#if !defined (MSGPARSER_H)
#define MSGPARSER_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

#include <Mail/Mime.h>
#include <stack>

// Message format based on RFC #2822
// MIME based on RFC #2045

class LineSeq;

namespace Pop3
{
	class Sink;

	class Parser
	{
	public:
		void Parse (LineSeq & lineSeq, Pop3::Sink & sink);
	private:
		void Message ();
		void Headers ();
		void Body ();
		void MultiPart ();
		void SimplePart ();
		void AppOctetStreamBase64Part ();
		void PlainTextPart ();
		void PlainTextMessage ();

		void EatToEnd ();
		bool EatToLine (std::string const & stopLine);
private:
		LineSeq		  * _lineSeq; 
		Pop3::Sink    * _sink;
		std::stack<MIME::Headers>	_context;
		MIME::Headers			  	_currentContext;
	};
};

#endif
