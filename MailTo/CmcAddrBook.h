#if !defined (CMCADDRBOOK_H)
#define CMCADDRBOOK_H
//
// (c) Reliable Software 1998
//

#include <windows.h>
#include <xcmc.h>

class Session;

class AddressBook
{
public:
	AddressBook (Session & session);

	CMC_recipient * Lookup (CMC_recipient const * recip) const;
	void Release (CMC_recipient * recip) const { _free (recip); }

private:
	typedef CMC_return_code (*CmcLookup) (CMC_session_id session,
										  CMC_recipient const * recipient_in,
										  CMC_flags lookup_flags,
										  CMC_ui_id ui_id,
										  CMC_uint32 * count,
										  CMC_recipient ** recipient_out,
										  CMC_extension * lookup_extension);
	typedef CMC_return_code (*CmcFree) (CMC_buffer buf);

	Session &	_session;
	CmcLookup	_lookup;
	CmcFree		_free;
};

#endif
