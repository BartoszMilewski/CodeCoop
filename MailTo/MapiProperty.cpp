//
// (c) Reliable Software 1998
//

#include "MapiProperty.h"
#include "MapiBuffer.h"

#include <mapix.h>

void AddressType::Serialize (MapiBufferSerializer<SPropValue> & out) const
{
	SPropValue mapiProp;
	memset (&mapiProp, 0, sizeof (SPropValue));
	mapiProp.ulPropTag = PR_ADDRTYPE;
	mapiProp.Value.lpszA = const_cast<char *>(_addrType.c_str ());
	out.Write (&mapiProp);
}

void DisplayName::Serialize (MapiBufferSerializer<SPropValue> & out) const
{
	SPropValue mapiProp;
	memset (&mapiProp, 0, sizeof (SPropValue));
	mapiProp.ulPropTag = PR_DISPLAY_NAME;
	mapiProp.Value.lpszA = const_cast<char *>(_displayName.c_str ());
	out.Write (&mapiProp);
}

void EmailAddress::Serialize (MapiBufferSerializer<SPropValue> & out) const
{
	SPropValue mapiProp;
	memset (&mapiProp, 0, sizeof (SPropValue));
	mapiProp.ulPropTag = PR_EMAIL_ADDRESS;
	mapiProp.Value.lpszA = const_cast<char *>(_emailAddress.c_str ());
	out.Write (&mapiProp);
}

void RecipientType::Serialize (MapiBufferSerializer<SPropValue> & out) const
{
	SPropValue mapiProp;
	memset (&mapiProp, 0, sizeof (SPropValue));
	mapiProp.ulPropTag = PR_RECIPIENT_TYPE;
	mapiProp.Value.ul = _recipientType;
	out.Write (&mapiProp);
}

void RowId::Serialize (MapiBufferSerializer<SPropValue> & out) const
{
	SPropValue mapiProp;
	memset (&mapiProp, 0, sizeof (SPropValue));
	mapiProp.ulPropTag = PR_ROWID;
	mapiProp.Value.ul = _rowId;
	out.Write (&mapiProp);
}
