#if !defined (MAPIPROPERTY_H)
#define MAPIPROPERTY_H
//
// (c) Reliable Software 1998 -- 2004
//

#include "LightString.h"
#include "MapiBuffer.h"

#include <mapix.h>

namespace Mapi
{
	class Property
	{
	public:
		operator SPropValue const * () const { return &_prop; }

	protected:
		SPropValue	_prop;
	};

	class StringProperty : public Property
	{
	public:
		StringProperty (std::string const & str)
			: _str (str)
		{
			_prop.Value.lpszA = const_cast<char *>(_str.c_str ());
		}

	protected:
		std::string	_str;
	};

	class BinaryProperty : public Property
	{
	public:
		BinaryProperty ()
		{
			_prop.Value.bin.cb = 0;
			_prop.Value.bin.lpb = 0;
		}
		BinaryProperty (std::vector<unsigned char> const & bin)
			: _bin (bin)
		{
			_prop.Value.bin.cb = _bin.size ();
			_prop.Value.bin.lpb = &_bin [0];
		}

		void Init (unsigned long count, unsigned char const * bytes);

	protected:
		std::vector<unsigned char>	_bin;
	};

	class AddressType : public StringProperty
	{
	public:
		AddressType (std::string const & adrType)
			: StringProperty (adrType)
		{
			_prop.ulPropTag = PR_ADDRTYPE;
		}
	};

	class DisplayName : public StringProperty
	{
	public:
		DisplayName (std::string const & name)
			: StringProperty (name)
		{
			_prop.ulPropTag = PR_DISPLAY_NAME;
		}
	};

	class Address : public StringProperty
	{
	public:
		Address (std::string const & emailAddress)
			: StringProperty (emailAddress)
		{
			_prop.ulPropTag = PR_EMAIL_ADDRESS;
		}
	};

	class Subject : public StringProperty
	{
	public:
		Subject (std::string const & subject)
			: StringProperty (subject)
		{
			_prop.ulPropTag = PR_SUBJECT;
		}
	};

	class NoteText : public StringProperty
	{
	public:
		NoteText (std::string const & subject)
			: StringProperty (subject)
		{
			_prop.ulPropTag = PR_BODY;
		}
	};

	class MessageClass : public StringProperty
	{
	public:
		MessageClass (std::string const & cls)
			: StringProperty (cls)
		{
			_prop.ulPropTag = PR_MESSAGE_CLASS;
		}
	};

	class FileName : public StringProperty
	{
	public:
		FileName (std::string const & fn)
			: StringProperty (fn)
		{
			_prop.ulPropTag = PR_ATTACH_FILENAME;
		}
	};

	class FileNameExtension : public StringProperty
	{
	public:
		FileNameExtension (std::string const & ext)
			: StringProperty (ext)
		{
			_prop.ulPropTag = PR_ATTACH_EXTENSION;
		}
	};

	class LongFileName : public StringProperty
	{
	public:
		LongFileName (std::string const & fn)
			: StringProperty (fn)
		{
			_prop.ulPropTag = PR_ATTACH_LONG_FILENAME;
		}
	};

	class AttachTagMime : public Property
	{
	public:
		AttachTagMime ();

	private:
		static unsigned char _oid [];
	};

	class MimeTag : public StringProperty
	{
	public:
		MimeTag (std::string const & tag)
			: StringProperty (tag)
		{
			_prop.ulPropTag = PR_ATTACH_MIME_TAG;
		}
	};

	class RecipientType : public Property
	{
	public:
		RecipientType (long recipType)
		{
			_prop.ulPropTag = PR_RECIPIENT_TYPE;
			_prop.Value.ul = recipType;
		}
	};

	class RowId : public Property
	{
	public:
		RowId (long rowId)
		{
			_prop.ulPropTag = PR_ROWID;
			_prop.Value.ul = rowId;
		}
	};

	class AttachmentNumber : public Property
	{
	public:
		AttachmentNumber (unsigned long num)
		{
			_prop.ulPropTag = PR_ATTACH_NUM;
			_prop.Value.ul = num;
		}
	};

	class AttachByValue : public Property
	{
	public:
		AttachByValue ()
		{
			_prop.ulPropTag = PR_ATTACH_METHOD;
			_prop.Value.ul = ATTACH_BY_VALUE;
		}
	};

	class MsgHasAttachment : public Property
	{
	public:
		MsgHasAttachment (bool hasAttachment)
		{
			_prop.ulPropTag = PR_HASATTACH;
			_prop.Value.b = hasAttachment ? TRUE : FALSE;
		}
	};

	class RenderingPosition : public Property
	{
	public:
		RenderingPosition (unsigned long pos)
		{
			_prop.ulPropTag = PR_RENDERING_POSITION;
			_prop.Value.ul = pos;
		}
	};

	class DeleteSentMsg : public Property
	{
	public:
		DeleteSentMsg (bool doDelete)
		{
			_prop.ulPropTag = PR_DELETE_AFTER_SUBMIT;
			_prop.Value.b = doDelete ? TRUE : FALSE;
		}
	};

	class SentItemsId : public BinaryProperty
	{
	public:
		SentItemsId ()
		{
			_prop.ulPropTag = PR_SENTMAIL_ENTRYID;
		}
		SentItemsId (std::vector<unsigned char> const & id)
			: BinaryProperty (id)
		{
			_prop.ulPropTag = PR_SENTMAIL_ENTRYID;
		}
		SentItemsId (unsigned long count, unsigned char const * bytes)
		{
			Init (count, bytes);
			_prop.ulPropTag = PR_SENTMAIL_ENTRYID;
		}

		bool IsValid () const { return !_bin.empty (); }
	};

	class PropertyList
	{
	public:
		PropertyList () {}
		void Add (unsigned long tag) { _tags.push_back (tag); }
		SPropTagArray * Cnv2TagArray ();

	private:
		std::vector<unsigned long>		_tags;
		std::unique_ptr<SPropTagArray>	_tagArray;
	};

	class RetrievedProperty
	{
	public:
		RetrievedProperty ()
			: _count (0),
			  _prop (0)
		{}
		~RetrievedProperty ()
		{
			if (_prop != 0)
				::MAPIFreeBuffer (_prop);
		}

		unsigned long * GetCountBuf () { return &_count; }
		SPropValue ** GetBuf () { return &_prop; }
		unsigned long GetCount () const { return _count; }

		SPropValue & operator [] (unsigned int idx) { return _prop [idx]; }

	private:
		unsigned long	_count;
		SPropValue *	_prop;
	};
}

#endif
