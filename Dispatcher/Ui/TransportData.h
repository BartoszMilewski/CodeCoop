#if !defined (TRANSPORTDATA_H)
#define TRANSPORTDATA_H
// ----------------------------------
// (c) Reliable Software, 2001 - 2005
// ----------------------------------

#include "TransportHeader.h"

// wrapper class used for displaying script transport information
class TransportData
{
public:
	TransportData (std::unique_ptr<TransportHeader> txHdr)
		: _txHdr (std::move(txHdr))
	{}

	bool IsPresent () const { return _txHdr.get () != 0; }
	bool IsEmpty () const { return _txHdr->IsEmpty (); }
	bool IsValid () const { return _txHdr->IsValid (); }

    std::string const & GetComment () const { return _comment; }
	void SetComment (std::string const & comment) { _comment = comment; }
    std::string const & GetStatus  () const { return _status; }
	void SetStatus (std::string const & status) { _status = status; }

	std::string const & GetProjectName () const { return _txHdr->GetProjectName (); }
	Address const & GetSenderAddress () const { return _txHdr->GetSenderAddress (); }
	AddresseeList const & GetRecipients () const { return _txHdr->GetRecipients (); }
	bool ToBeForwarded () const { return _txHdr->ToBeForwarded (); }
	bool IsDefectScript () const { return _txHdr->IsDefectScript (); }

private:
	std::unique_ptr<TransportHeader> const _txHdr;
	std::string			_comment;
	std::string			_status;
};

#endif
