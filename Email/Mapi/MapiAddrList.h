#if !defined (MAPIADDRESSLIST_H)
#define MAPIADDRESSLIST_H
//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

#include "MapiBuffer.h"

namespace Mapi
{
	class Recipient
	{
	public:
		int	GetPropertyCount () const { return 2; }
		void MakeSubmitted ();
		LPSPropValue ReleaseProps () { return _props.Release (); }

	protected:
		Recipient (std::string const & displayName, unsigned long type);

	private:
		Buffer<SPropValue>	_props;
	};

	class ToRecipient : public Recipient
	{
	public:
		ToRecipient (std::string const & displayName)
			: Recipient (displayName, MAPI_TO)
		{}
	};

	class CcRecipient : public Recipient
	{
	public:
		CcRecipient (std::string const & displayName)
			: Recipient (displayName, MAPI_CC)
		{}
	};

	class BccRecipient : public Recipient
	{
	public:
		BccRecipient (std::string const & displayName)
			: Recipient (displayName, MAPI_BCC)
		{}
	};

	class AddrList
	{
	public:
		AddrList ()
			: _addrList (0),
			  _top (0)
		{}
		AddrList (int recipientCount);
		AddrList (AddrList & buf);
		~AddrList ();

		AddrList & operator = (AddrList & buf);
		void AddRecipient (Recipient & recipient);
		LPADRLIST GetBuf () const { return _addrList; }

		class Sequencer
		{
		public:
			Sequencer (AddrList const & addrList)
				: _addrList (addrList.GetBuf ()),
				  _curEntry (0)
			{}

			bool AtEnd () const { return _curEntry == _addrList->cEntries; }
			void Advance () { ++_curEntry; }

			char const * GetDisplayName () const;
			char const * GetEmailAddr () const;

		private:
			char const * GetStringProperty (unsigned long propTag) const;

		private:
			LPADRLIST		_addrList;
			unsigned int	_curEntry;
		};

		void Dump (char const * title) const;

	private:
		LPADRLIST Release ();
		void Free ();

	private:
		LPADRLIST		_addrList;
		unsigned int	_top;
	};

};

#endif
