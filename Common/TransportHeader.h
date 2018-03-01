#if !defined (TRANSPORTHEADER_H)
#define TRANSPORTHEADER_H
//----------------------------------
// (c) Reliable Software 1998 - 2006
// ---------------------------------

#include "ScriptSerialize.h"
#include "SerString.h"
#include "Params.h"
#include "Addressee.h"
#include "Address.h"
#include "GlobalId.h"

#include <File/MemFile.h>

class TransportHeader : public ScriptSerializable
{
public:
	TransportHeader ()
		: _scriptId (gidInvalid),
		  _toBeForwarded (true),
		  _recipientCount (0),
          _isEmpty (false),
		  _isValid (true)
	{}
	TransportHeader (GlobalId scriptId, bool isPureControl, bool isDefectOrRemove)
		: _scriptId (scriptId),
		  _toBeForwarded (true),
		  _recipientCount (0),
          _isEmpty (false),
		  _isValid (true)
	{
		SetControlScript (isPureControl);
		SetDefect (isDefectOrRemove);
	}
    TransportHeader (Deserializer & in);

	void AddScriptId (GlobalId id) { _scriptId = id; }
	void AddSender (Address const & address) { _sender = address; }
	void AddRecipients (AddresseeList const & recipients);
	void SetDefect (bool isDefectScript) { _flags.SetDefect (isDefectScript); }
	void SetControlScript (bool isControlScript) { _flags.SetControl (isControlScript); }
	void SetDispatcherAddendum (bool flag) { _flags.SetAddendum (flag); }
	void SetBccRecipients (bool flag) { _flags.SetBccRecipients (flag); }
	void SetInvitation (bool flag) { _flags.SetInvitation (flag); }
	void SetForward (bool forward) { _toBeForwarded = forward; }

	GlobalId GetScriptId () const { return _scriptId; }
	bool IsEmpty () const { return _isEmpty; }
	bool IsValid () const { return _isValid; }
    bool ToBeForwarded () const { return _toBeForwarded; }
    bool IsDefectScript () const { return VersionNo () < 29 ? false : _flags.IsDefect (); }
	bool IsControlScript () const { return _flags.IsControl (); }
	bool IsDispatcherAddendum () const { return _flags.IsAddendum (); }
	bool UseBccRecipients () const { return _flags.UseBccRecipients (); }
	bool IsInvitation () const { return _flags.IsInvitation (); }
	Address const & GetSenderAddress () const { return _sender; }
	std::string const & GetProjectName () const { return _sender.GetProjectName (); }

	AddresseeList const & GetRecipients () const { return _recipients; }

    int  VersionNo () const { return scriptVersion; }
    int  SectionId () const { return 'TXHR'; }
    bool IsSection () const { return true; }

protected:
    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);
    bool ConversionSupported (int versionRead) const { return versionRead >= 26; }

private:
    class SerializeFlags
    {
    public:
        SerializeFlags (Serializer & out) : _out (out) {}
        void operator () (Addressee const & addressee);
    private:
        Serializer & _out;
    };

	class TxFlags : public Serializable
	{
	public:
		TxFlags () : _value (0) {}
		unsigned long GetValue () const { return _value; } 
		void Reset () { _value = 0; }

		void SetDefect (bool bit) { _bits._defect = bit; }
		void SetAddendum (bool bit) { _bits._addendum = bit; }
		void SetControl (bool bit) { _bits._control = bit; }
		void SetBccRecipients (bool bit) { _bits._bcc = bit; }
		void SetInvitation (bool bit) { _bits._invitation = bit; }

		bool IsDefect () const { return _bits._defect != 0; }
		bool IsAddendum () const { return _bits._addendum != 0; }
		bool IsControl () const { return _bits._control != 0; }
		bool UseBccRecipients () const { return _bits._bcc != 0; }
		bool IsInvitation () const { return _bits._invitation != 0; }

		void Serialize (Serializer & out) const { out.PutLong (_value); }
		void Deserialize (Deserializer & in, int version) { _value = in.GetLong (); }

	private:
		union
		{
			unsigned long _value;			// for quick serialization
			struct
			{
				unsigned long _defect:1;	// Code Co-op project defect script
				unsigned long _addendum:1;	// Dispatcher addendum present in the script file
				unsigned long _control:1;	// Control script, but not FS or Package
				unsigned long _bcc:1;		// Bcc recipients
				unsigned long _invitation:1;// Invitation
			} _bits;
		};
	};

private:
    // Persistent
	GlobalId			_scriptId;
    bool				_toBeForwarded;
	TxFlags				_flags;
	Address				_sender;
    int					_recipientCount;
    AddresseeList   	_recipients;
	// Volatile
	bool				_isEmpty;
	bool				_isValid;
};

class MemMappedHeader : MemFileExisting
{
public:
	MemMappedHeader (char const * fullName, File::Mode mode = File::OpenExistingMode())
        : MemFileExisting (fullName, mode)
    {}
    void StampDelivery (int addresseeIdx);
    void StampForwardFlag (bool on);
};

//
// Used by Dispatcher for partial deserialization of a ScriptHeader
// Contains script comment

class ScriptSubHeader: public ScriptSerializable
{
public:
	ScriptSubHeader (Deserializer & in)
	{
		Read (in);
	}
	// This constructor is used for creating dispatcher-to-dispatcher scripts
	ScriptSubHeader (std::string const & comment)
		: _comment (comment),
		_partNumber (1),
		_partCount (1),
		_maxChunkSize (0)
	{}

	bool IsSection () const { return true; }
	int  SectionId () const { return 'SHDR'; }
	int  VersionNo () const { return scriptVersion; }

	unsigned GetPartNumber () const { return _partNumber; }
	unsigned GetPartCount () const { return _partCount; }
	unsigned GetMaxChunkSize () const { return _maxChunkSize; }
	std::string const & GetComment () const { return _comment; }

	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

private:
	unsigned	_partNumber;	// for chunks
	unsigned	_partCount;
	unsigned	_maxChunkSize;
	SerString   _comment;
};

std::ostream & operator<<(std::ostream & os, TransportHeader const & txHdr);

#endif
