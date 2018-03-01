#if !defined (MAPIMESSAGE_H)
#define MAPIMESSAGE_H
//-----------------------------------------------------
//  (c) Reliable Software 2001 -- 2004
//-----------------------------------------------------

#include "MapiIface.h"

class SafePaths;

namespace Mapi
{
	class Folder;
	class AddrList;
	class FolderEntryId;

	class Message
	{
	public:
		explicit Message (Folder & folder);
		Message (Folder & folder, std::vector<unsigned char> const & id);
		~Message ();

		void GetAttachmentTable (Interface<IMAPITable> & table);
		void OpenAttachment (unsigned long attNum, Interface<IAttach> & attach);
		void CreateAttachment (unsigned long & attNum, Interface<IAttach> & attach);
		void ReadEnvelope (std::string & subject, std::string & sender, std::string & email);
		void GetId (Buffer<SPropValue> & buf);
		void AddRecipients (AddrList const & addrList);
		void AddAttachment (std::string const & path);
		void SetSubject (std::string const & subject);
		void SetNoteText (std::string const & noteText);
		void SetClass (std::string const & cls);
		void MarkHasAttachment ();
		void SetSentOptions (FolderEntryId const & sentItemsId);
		void SaveChanges ();
		void MarkAsRead ();
		void Submit ();
	private:
		Message (Message const &);
		Message & operator= (Message const &);
	private:
		void SetProperty (SPropValue const * prop);
	private:
		Interface<IMessage>	_msg;
	};

	class Envelope
	{
	public:
		explicit Envelope (Message & msg);

		std::string const & GetSubject () const { return _subject; }
		std::string const & GetSenderName () const { return _senderName; }
		std::string const & GetSenderEmail () const { return _senderEmail; }

	private:
		std::string	_subject;
		std::string	_senderName;
		std::string	_senderEmail;
	};

	class AttachmentTable
	{
	public:
		explicit AttachmentTable (Message & msg);
		void SaveAttachments (SafePaths & attPaths);
	private:
		AttachmentTable (AttachmentTable const &);
		AttachmentTable & operator= (AttachmentTable const &);
	private:
		Message &				_msg;
		Interface<IMAPITable>	_table;
	};

	class Attachment
	{
	public:
		Attachment (Message & msg, unsigned long attachNum);
		explicit Attachment (Message & msg);

		void SaveChanges ();
		unsigned long GetId () const { return _attNum; }
		bool IsFile ();

		std::string GetFileName ();
		void Save (char const * path);
		void Store (std::string const & path);
	private:
		Attachment (Attachment const &);
		Attachment & operator= (Attachment const &);
	private:
		void SetProperty (SPropValue const * prop);
		void CreateBuffer (Com::UnknownPtr & stream);
		void OpenBuffer (Com::UnknownPtr & stream);
	private:
		class File : public Com::IfacePtr<IStream>
		{
		public:
			File () {}
			void Open (char const * path);
			void Create (char const * path);
			void Write (Com::IfacePtr<IStream> & buf, unsigned long size);
			void Read (Com::IfacePtr<IStream> & buf);
		};

		class Buffer : public Com::IfacePtr<IStream>
		{
		public:
			Buffer () {}
			void Open (Attachment & attach);
			void Create (Attachment & attach);
			unsigned long size ();
		};

	private:
		Interface<IAttach>	_attach;
		unsigned long		_attNum;
	};
}

#endif
