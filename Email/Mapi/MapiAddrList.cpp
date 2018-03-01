//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

#include "precompiled.h"
#include "MapiAddrList.h"

#include <mapiutil.h>

#include <Dbg/Assert.h>

#if !defined (NDEBUG)
#include <LightString.h>

void DumpProps (int count, SPropValue const * props, Msg & info);
void DumpProperty (SPropValue const & prop, Msg & info);
void DumpBinary (unsigned char * data, unsigned int size, Msg & info);
#endif

using namespace Mapi;

AddrList::AddrList (int recipientCount)
	: _top (0)
{
	unsigned int size = recipientCount * sizeof (ADRENTRY) + sizeof (ULONG);
	SCODE sRes = ::MAPIAllocateBuffer (size, reinterpret_cast<LPVOID *>(&_addrList));
	if (FAILED (sRes))
		throw Win::Exception ("Cannot allocate memory for MAPI address list");
	memset (_addrList, 0 , size);		
	_addrList->cEntries = recipientCount;
}

AddrList::~AddrList ()
{
	Free ();
}

void AddrList::AddRecipient (Recipient & recipient)
{
	Assert (_top < _addrList->cEntries);
	_addrList->aEntries [_top].cValues = recipient.GetPropertyCount ();
	_addrList->aEntries [_top].rgPropVals = recipient.ReleaseProps ();
	_top++;
}

void AddrList::Dump (char const * title) const
{
#if !defined (NDEBUG)
	Msg info;
	info << _addrList->cEntries << " entries:\n";
	for (unsigned int i = 0; i < _addrList->cEntries; i++)
	{
		info << _addrList->aEntries [i].cValues << " properties at address 0x" << std::hex << _addrList->aEntries [i].rgPropVals << "\n";
		DumpProps (_addrList->aEntries [i].cValues, _addrList->aEntries [i].rgPropVals, info);
	}
	MessageBox (0, info.c_str (), title, MB_ICONEXCLAMATION | MB_OK);
#endif
}

struct _ADRLIST * AddrList::Release ()
{
	LPADRLIST tmp = _addrList;
	_addrList = 0;
	return tmp;
}

void AddrList::Free ()
{
	if (_addrList != 0)
	{
		::FreePadrlist (_addrList);
		_addrList = 0;
	}
}

char const * AddrList::Sequencer::GetDisplayName () const
{
	return GetStringProperty (PR_DISPLAY_NAME);
}

char const * AddrList::Sequencer::GetEmailAddr () const
{
	return GetStringProperty (PR_EMAIL_ADDRESS);
}

char const * AddrList::Sequencer::GetStringProperty (unsigned long propTag) const
{
	unsigned int propCount = _addrList->aEntries [_curEntry].cValues;
	SPropValue const * props = _addrList->aEntries [_curEntry].rgPropVals;
	for (unsigned int i = 0; i < propCount; ++i)
	{
		if (props [i].ulPropTag == propTag)
			return props [i].Value.lpszA;
	}
	return 0;
}

Recipient::Recipient (std::string const & displayName, unsigned long type)
	: _props (GetPropertyCount ())
{
	_props [0].ulPropTag = PR_RECIPIENT_TYPE;
	_props [0].Value.ul = type;
	_props [1].ulPropTag = PR_DISPLAY_NAME;
	Buffer<char> name (displayName.length () + 1);	// For zero at the end
	strcpy (name.GetBuf (), displayName.c_str ());
	_props [1].Value.lpszA = name.Release ();
}

void Recipient::MakeSubmitted ()
{
	_props [0].Value.ul |= MAPI_SUBMITTED;
}


//
// Debug helpers
//

#if !defined (NDEBUG)

void DumpProps (int count, SPropValue const * props, Msg & info)
{
	for (int i = 0; i < count; i++)
	{
		DumpProperty (props [i], info);
	}
}

void DumpProperty (SPropValue const & prop, Msg & info)
{
	switch (prop.ulPropTag)
	{
	case PR_ADDRTYPE:
		info << "PR_ADDRTYPE: " << prop.Value.lpszA;
		break;
	case PR_DISPLAY_NAME:
		info << "PR_DISPLAY_NAME: " << prop.Value.lpszA;
		break;
	case PR_EMAIL_ADDRESS:
		info << "PR_EMAIL_ADDRESS: " << prop.Value.lpszA;
		break;
	case PR_RECIPIENT_TYPE:
		info << "PR_RECIPIENT_TYPE: ";
		switch ((prop.Value.ul & ~MAPI_SUBMITTED))
		{
		case MAPI_TO:
			info << "MAPI_TO";
			break;
		case MAPI_CC:
			info << "MAPI_CC";
			break;
		case MAPI_BCC:
			info << "MAPI_BCC";
			break;
		case MAPI_P1:
			info << "MAPI_P1";
			break;
		}
		if (prop.Value.ul & MAPI_SUBMITTED)
			info << " and MAPI_SUBMITTED";
		break;
	case PR_ENTRYID:
		{
			info << "PR_ENTRYID; length = " << prop.Value.bin.cb << " bytes\n";
			ENTRYID * entry = reinterpret_cast<ENTRYID *>(prop.Value.bin.lpb);
			unsigned int i = entry->abFlags [0];
			info << "Flags: 0x" << std::hex << i;
			if (entry->abFlags [0] & MAPI_NOTRECIP)
				info << " MAPI_NOTRECIP";
			if (entry->abFlags [0] & MAPI_NOTRESERVED)
				info << " MAPI_NOTRESERVED";
			if (entry->abFlags [0] & MAPI_NOW)
				info << " MAPI_NOW";
			if (entry->abFlags [0] & MAPI_SHORTTERM)
				info << " MAPI_SHORTTERM";
			if (entry->abFlags [0] & MAPI_THISSESSION)
				info << " MAPI_THISSESSION";
			info << "\nId: ";
			DumpBinary (&entry->ab [0], prop.Value.bin.cb - sizeof (entry->abFlags), info);
		}
		break;
	case PR_OBJECT_TYPE:
		info << "PR_OBJECT_TYPE: ";
		switch (prop.Value.ul)
		{
		case MAPI_ABCONT:
			info << "Address book container";
			break;
		case MAPI_ADDRBOOK:
			info << "Address book";
			break;
		case MAPI_ATTACH:
			info << "Message attachment";
			break;
		case MAPI_DISTLIST:
			info << "Distribution list";
			break;
		case MAPI_FOLDER:
			info << "Folder";
			break;
		case MAPI_FORMINFO:
			info << "Form";
			break;
		case MAPI_MAILUSER:
			info << "Messaging user";
			break;
		case MAPI_MESSAGE:
			info << "Message";
			break;
		case MAPI_PROFSECT:
			info << "Profile section";
			break;
		case MAPI_STATUS:
			info << "Status";
			break;
		case MAPI_STORE:
			info << "Store";
			break;
		default:
			info << "Unknown - 0x" << std::hex << prop.Value.ul;
			break;
		}
		break;
	case PR_DISPLAY_TYPE:
		info << "PR_DISPLAY_TYPE: ";
		switch (prop.Value.ul)
		{
		case DT_AGENT:
			info << "An automated agent";
			break;
		case DT_DISTLIST:
			info << "A distribution list.";
			break;
		case DT_FOLDER:
			info << "Display default folder icon adjacent to folder.";
			break;
		case DT_FOLDER_LINK:
			info << "Display default folder link icon adjacent to folder rather than the default folder icon.";
			break;
		case DT_FOLDER_SPECIAL: 
			info << "Display icon for a folder with an application-specific distinction, such as a special type of public folder.";
			break;
		case DT_FORUM:
			info << "A forum, such as a bulletin board service or a public or shared folder.";
			break;
		case DT_GLOBAL:
			info << "A global address book.";
			break;
		case DT_LOCAL:
			info << "A local address book that you share with a small workgroup.";
			break;
		case DT_MAILUSER:
			info << "A typical messaging user.";
			break;
		case DT_MODIFIABLE:
			info << "Modifiable; the container should be denoted as modifiable in the user interface.";
			break;
		case DT_NOT_SPECIFIC:
			info << "Does not match any of the other settings.";
			break;
		case DT_ORGANIZATION:
			info << "A special alias defined for a large group (organization).";
			break;
		case DT_PRIVATE_DISTLIST:
			info << "A private, personally administered distribution list.";
			break;
		case DT_REMOTE_MAILUSER:
			info << "A recipient known to be from a foreign or remote messaging system.";
			break;
		case DT_WAN:
			info << "A wide area network address book.";
			break;
		}
		break;
	case PR_SEND_RICH_INFO:
		info << "PR_SEND_RICH_INFO: 0x" << std::hex << prop.Value.b;
		break;
	case PR_SEARCH_KEY:
		info << "PR_SEARCH_KEY: ";
		DumpBinary (prop.Value.bin.lpb, prop.Value.bin.cb, info);
		break;
	case PR_RECORD_KEY:
		info << "PR_RECORD_KEY: ";
		DumpBinary (prop.Value.bin.lpb, prop.Value.bin.cb, info);
		break;
	case PR_TRANSMITABLE_DISPLAY_NAME:
		info << "PR_TRANSMITABLE_DISPLAY_NAME: " << prop.Value.lpszA;
		break;
	case PR_PROVIDER_DISPLAY:
		info << "PR_PROVIDER_DISPLAY: " << prop.Value.lpszA;
		break;
	case PR_RESOURCE_FLAGS:
		info << "PR_RESOURCE_FLAGS:";
		if (prop.Value.ul & SERVICE_DEFAULT_STORE)
			info << " SERVICE_DEFAULT_STORE";
		if (prop.Value.ul & SERVICE_NO_PRIMARY_IDENTITY)
			info << " SERVICE_NO_PRIMARY_IDENTITY";
		if (prop.Value.ul & SERVICE_PRIMARY_IDENTITY)
			info << " SERVICE_PRIMARY_IDENTITY";
		if (prop.Value.ul & SERVICE_SINGLE_COPY)
			info << " SERVICE_SINGLE_COPY";
		if (prop.Value.ul & STATUS_DEFAULT_OUTBOUND)
			info << " STATUS_DEFAULT_OUTBOUND";
		if (prop.Value.ul & STATUS_DEFAULT_STORE)
			info << " STATUS_DEFAULT_STORE";
		if (prop.Value.ul & STATUS_NEED_IPM_TREE)
			info << " STATUS_NEED_IPM_TREE";
		if (prop.Value.ul & STATUS_NO_DEFAULT_STORE)
			info << " STATUS_NO_DEFAULT_STORE";
		if (prop.Value.ul & STATUS_NO_PRIMARY_IDENTITY)
			info << " STATUS_NO_PRIMARY_IDENTITY";
		if (prop.Value.ul & STATUS_OWN_STORE)
			info << " STATUS_OWN_STORE";
		if (prop.Value.ul & STATUS_PRIMARY_STORE)
			info << " STATUS_PRIMARY_STORE";
		if (prop.Value.ul & STATUS_SECONDARY_STORE)
			info << " STATUS_SECONDARY_STORE";
		if (prop.Value.ul & STATUS_SIMPLE_STORE)
			info << " STATUS_SIMPLE_STORE";
		if (prop.Value.ul & STATUS_TEMP_SECTION)
			info << " STATUS_TEMP_SECTION";
		if (prop.Value.ul & STATUS_XP_PREFER_LAST)
			info << " STATUS_XP_PREFER_LAST";
		break;
	case PR_DEFAULT_STORE:
		info << "PR_DEFAULT_STORE: 0x" << std::hex << prop.Value.b;
		break;
	case PR_MDB_PROVIDER:
		info << "PR_MDB_PROVIDER: ";
		DumpBinary (prop.Value.bin.lpb, prop.Value.bin.cb, info);
		break;
	case PR_INSTANCE_KEY:
		info << "PR_INSTANCE_KEY: ";
		DumpBinary (prop.Value.bin.lpb, prop.Value.bin.cb, info);
		break;
	case PR_RESOURCE_TYPE:
		info << "PR_RESOURCE_TYPE: ";
		switch (prop.Value.ul)
		{
		case MAPI_AB:
			info << "Address book";
			break;
		case MAPI_AB_PROVIDER: 
			info << "Address book provider";
			break;
		case MAPI_HOOK_PROVIDER:
			info << "Spooler hook provider";
			break;
		case MAPI_PROFILE_PROVIDER:
			info << "Profile provider";
			break;
		case MAPI_SPOOLER:
			info << "Message spooler";
			break;
		case MAPI_STORE_PROVIDER:
			info << "Message store provider";
			break;
		case MAPI_SUBSYSTEM:
			info << "Internal MAPI subsystem ";
			break;
		case MAPI_TRANSPORT_PROVIDER:
			info << "Transport provider";
			break;
		default:
			info << "Unknown - 0x" << std::hex << prop.Value.ul;
			break;
		}
		break;
	case PR_DEPTH:
		info << "PR_DEPTH: " << prop.Value.ul;
		break;
	default:
		info << "Id: 0x" << std::hex << PROP_ID (prop.ulPropTag) << "; Type: ";
		switch (PROP_TYPE (prop.ulPropTag))
		{
		case PT_SHORT:
			info << "short int; value: 0x" << prop.Value.i;
			break;
		case PT_LONG:
			info << "long; value: 0x" << prop.Value.ul;
			break; 
		case PT_FLOAT:
			info << "float; value: " << prop.Value.flt;
			break; 
		case PT_DOUBLE:
			info << "double; value: " << prop.Value.dbl;
			break; 
		case PT_BOOLEAN:
			info << "boolean; value: " << prop.Value.b;
			break; 
		case PT_CURRENCY:
			info << "PT_CURRENCY";
			break;  
		case PT_APPTIME:
			info << "PT_APPTIME";
			break; 
		case PT_SYSTIME:
			info << "PT_SYSTIME";
			break;  
		case PT_STRING8:
			info << "string; value: " << prop.Value.lpszA;
			break; 
		case PT_BINARY:
			info << "PT_BINARY";
			break;  
		case PT_UNICODE:
			info << "unicode string; value: " << prop.Value.lpszW;
			break; 
		case PT_CLSID:
			info << "PT_CLSID";
			break;  
		case PT_LONGLONG:
			info << "PT_LONGLONG";
			break;  
		case PT_MV_I2:
			info << "PT_MV_I2";
			break; 
		case PT_MV_R4:
			info << "PT_MV_R4";
			break; 
		case PT_MV_DOUBLE:
			info << "PT_MV_DOUBLE";
			break; 
		case PT_MV_CURRENCY:
			info << "PT_MV_CURRENCY";
			break; 
		case PT_MV_APPTIME:
			info << "PT_MV_APPTIME";
			break; 
		case PT_MV_SYSTIME:
			info << "PT_MV_SYSTIME";
			break; 
		case PT_MV_BINARY:
			info << "PT_MV_BINARY";
			break; 
		case PT_MV_STRING8:
			info << "PT_MV_STRING8";
			break; 
		case PT_MV_UNICODE:
			info << "PT_MV_UNICODE";
			break; 
		case PT_MV_CLSID:
			info << "PT_MV_CLSID";
			break; 
		case PT_MV_I8:
			info << "PT_MV_I8";
			break; 
		case PT_ERROR:
			info << "PT_ERROR";
			break;  
		case PT_NULL:
			info << "PT_NULL";
			break;
		case PT_OBJECT:
			info << "PT_OBJECT";
			break;
		default:
			info << "unknown - 0x" << std::hex << PROP_TYPE (prop.ulPropTag);
			break;
		}
		break;
	}
	info << "\n";
}

void DumpBinary (unsigned char * data, unsigned int size, Msg & info)
{
	for (unsigned int i = 0; i < size; i++)
	{
		if (isprint (data [i]))
		{
			char c = data [i];
			info << c;
		}
		else
		{
			unsigned int j = data [i];
			info << " 0x" << std::hex << j << " ";
		}
	}
}

#endif
