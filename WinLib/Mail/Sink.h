#if !defined (POP3SINK_H)
#define POP3SINK_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

#include <GenericIo.h>

namespace Pop3
{
	class Sink
	{
	public:
		virtual ~Sink () {}
		virtual void OnHeadersStart () = 0;
		virtual void OnHeader (
			std::string const & name,
			std::string const & value,
			std::string const & attribute) = 0;
		virtual void OnHeadersEnd () = 0;
		virtual void OnBodyStart () = 0;
		virtual void OnBodyEnd () = 0;

		virtual void OnAttachment (GenericInput<'\0'> & input, std::string const & filename) = 0;
		virtual void OnText (std::string const & text) = 0;
	};
};

#endif
