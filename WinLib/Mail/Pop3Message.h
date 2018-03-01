#if !defined (POP3_MESSAGE_H)
#define POP3_MESSAGE_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

#include "Sink.h"
#include <File/SerialUnVector.h>

class FilePath;
class SafePaths;

namespace Pop3
{
	class Attachment
	{
	public:
		Attachment (std::string const & name)
			: _name (name)
		{}

		typedef SerialUnVector<char> Storage;

		std::string const & GetName () const { return _name; }
		Storage & GetStorage () { return _contents; }
		void Save (FileIo & outFile) const { _contents.Save (outFile); }

	private:
		std::string		const _name;
		Storage				  _contents;
	};

	class Message : public Pop3::Sink
	{
	public:
		std::string const & GetSubject () const { return _subject; }
		void SaveAttachments (FilePath const & destFolder, SafePaths & attPaths) const;
		std::string const & GetText () const { return _text; }
		// Sink interface
		void OnHeadersStart ();
		void OnHeader (
			std::string const & name,
			std::string const & value,
			std::string const & attribute);
		void OnHeadersEnd ();
		void OnBodyStart ();
		void OnBodyEnd ();
		void OnAttachment (GenericInput<'\0'> & input, std::string const & filename);
		void OnText (std::string const & text);
	private:
		std::string					    _subject;
		auto_vector<Pop3::Attachment>	_attachments;
		std::string						_text;
	};
};

#endif
