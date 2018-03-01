#if !defined (SIMPLEMAPI_H)
#define SIMPLEMAPI_H
//------------------------------------
//  SimpleMAPI.h
//  (c) Reliable Software, 1999, 2000
//------------------------------------

#include <mapi.h>

// Simple MAPI API pointers
 
typedef ULONG (FAR PASCAL *MapiFindNext) (LHANDLE session,
										  ULONG ulUIParam,
										  LPTSTR lpszMessageType,
										  LPTSTR lpszSeedMessageID,
										  FLAGS flFlags,
										  ULONG reserved,
										  LPTSTR lpszMessageID);
typedef ULONG (FAR PASCAL *MapiReadMail) (LHANDLE session,
										  ULONG ulUIParam,
										  LPTSTR lpszMessageID,
										  FLAGS flFlags,
										  ULONG reserved,
										  lpMapiMessage FAR * lppMessage);
typedef ULONG (FAR PASCAL *MapiDeleteMail) (LHANDLE session,
											ULONG ulUIParam,
											LPTSTR lpszMessageID,
											FLAGS flFlags,
											ULONG reserved);
typedef ULONG (FAR PASCAL *MapiSaveMail) (LHANDLE lhSession,
										  ULONG ulUIParam,
										  lpMapiMessage lpMessage,
										  FLAGS flFlags,
										  ULONG ulReserved,
										  LPTSTR lpszMessageID);
typedef ULONG (FAR PASCAL *Send) (LHANDLE session,
								  ULONG ulUIParam,
								  lpMapiMessage message,
								  FLAGS flFlags,
								  ULONG ulReserved);
typedef ULONG (FAR PASCAL *MapiFree) (LPVOID buf);

// Helper classes

class RetrievedMessage
{
public:
	RetrievedMessage (MapiFree free)
		: _message (0),
		  _free (free)
	{}
	~RetrievedMessage ()
	{
		if (IsRetrieved ())
			_free (_message);
	}

	bool IsRetrieved () const { return _message != 0; }
	MapiMessage ** GetAddr () { return &_message; }
	MapiMessage * operator-> () { return _message; }

private:
	MapiFree		_free;
	MapiMessage *	_message;
};

#endif
