#if !defined (MAPIHELPERS_H)
#define MAPIHELPERS_H
//-----------------------------------------------------
//  MapiHelpers.h
//  (c) Reliable Software 2001 -- 2003
//-----------------------------------------------------

#include <mapix.h>

namespace Mapi
{
	class Result
	{
	public:
		Result ()
			: _hRes (S_OK)
		{}
		Result (HRESULT hRes)
			: _hRes (hRes)
		{}
		Result (Result const & result)
			: _hRes (result._hRes)
		{}

		operator HRESULT () const { return _hRes; }
		void operator = (HRESULT hRes) { _hRes = hRes; }
		bool IsError () const { return FAILED (_hRes); }
		bool IsOk () const { return SUCCEEDED (_hRes); }
		bool IsWarningNoService () const { return _hRes == MAPI_W_NO_SERVICE; }
		bool IsExtendedError () const { return _hRes == MAPI_E_EXTENDED_ERROR; }
		bool IsTooComplex () const { return _hRes == MAPI_E_TOO_COMPLEX; }
		bool ChangesDetected () const { return _hRes == MAPI_E_OBJECT_CHANGED; }
		bool OneProviderFailed () const { return _hRes == MAPI_E_FAILONEPROVIDER; }

	private:
		HRESULT	_hRes;
	};

	class ObjectType
	{
	public:
		ObjectType ()
			: _objType (0)
		{}

		unsigned long * operator & () { return &_objType; }
		bool IsMsgStore () const { return _objType == MAPI_STORE; }
		bool IsAddrBook () const { return _objType == MAPI_ADDRBOOK; }
		bool IsFolder () const { return _objType == MAPI_FOLDER; }
		bool IsAddrBookContainer () const { return _objType == MAPI_ABCONT; }
		bool IsMessage () const { return _objType == MAPI_MESSAGE; }
		bool IsMailUser () const { return _objType == MAPI_MAILUSER; }
		bool IsAttachment () const { return _objType == MAPI_ATTACH; }
		bool IsDistList () const { return _objType == MAPI_DISTLIST; }
		bool IsProfile () const { return _objType == MAPI_PROFSECT; }
		bool IsStatus () const { return _objType == MAPI_STATUS; }
		bool IsSession () const { return _objType == MAPI_SESSION; }
		bool IsForm () const { return _objType == MAPI_FORMINFO; }

	private:
		unsigned long	_objType;
	};

	class FolderMask
	{
	public:
		FolderMask ()
			: _mask (0)
		{}
		FolderMask (unsigned long mask)
			: _mask (mask)
		{}
		FolderMask (FolderMask const & mask)
			: _mask (mask._mask)
		{}

		void operator = (unsigned long mask) { _mask = mask; }
		bool IsOutboxFolderValid () const { return (_mask & FOLDER_IPM_OUTBOX_VALID) != 0; }
		bool IsWasteBasketFolderValid () const { return (_mask & FOLDER_IPM_WASTEBASKET_VALID) != 0; }
		bool IsSentItemsFolderValid () const { return (_mask & FOLDER_IPM_SENTMAIL_VALID) != 0; }

	private:
		unsigned long	_mask;
	};
}

#endif
