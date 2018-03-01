#if !defined (PROJECTINVITEDATA_H)
#define PROJECTINVITEDATA_H
//------------------------------------
//  (c) Reliable Software, 2006 - 2008
//------------------------------------

#include "CopyRequest.h"
#include "OpenSaveFileDlg.h"

#include <Bit.h>

namespace Project
{
	class InviteData
	{
	public:
		InviteData (std::string const & projectName);
		~InviteData ();

		std::string const & GetProjectName () const { return _projectName; }
		std::string const & GetUserName () const { return _userName; }
		std::string const & GetEmailAddress () const { return _emailAddress; }
		std::string const & GetComputerName () const { return _computerName; }
		std::string const & GetLocalFolder () const { return _copyRequest.GetLocalFolder (); }
		Ftp::SmartLogin & GetFtpLogin () { return _copyRequest.GetFtpLogin (); }
		Ftp::SmartLogin const & GetFtpLogin () const { return _copyRequest.GetFtpLogin (); }

		bool IsOnSatellite () const { return _options.test (InviteeOnSatellite); }
		bool IsObserver () const { return _options.test (InviteObserver); }
		bool IsCheckoutNotification () const { return _options.test (CheckoutNotification); }
		bool IsManualInvitationDispatch () const { return _options.test (ManualInvitationDispatch); }
		bool IsTransferHistory () const { return _options.test (TransferHistory); }
		bool IsStoreOnMyComputer () const { return _copyRequest.IsMyComputer (); }
		bool IsStoreOnLAN () const { return _copyRequest.IsLAN (); }
		bool IsStoreOnInternet () const { return _copyRequest.IsInternet (); }
		bool IsAnonymousLogin () const { return _copyRequest.IsAnonymousLogin (); }

		void SetUserName (std::string const & user)
		{
			_userName = user;
			_options.set (ChangesDetected, true);
		}
		void SetEmailAddress (std::string const & address)
		{
			_emailAddress = address;
			_options.set (ChangesDetected, true);
		}
		void SetComputerName (std::string const & computer)
		{
			_computerName = computer;
			_options.set (ChangesDetected, true);
		}
		void SetLocalFolder (std::string const & folder)
		{
			_copyRequest.SetLocalFolder (folder);
			_options.set (ChangesDetected, true);
		}

		void SetIsOnSatellite (bool bit)
		{
			_options.set (InviteeOnSatellite, bit);
			_options.set (ChangesDetected, true);
		}
		void SetIsObserver (bool bit)
		{
			_options.set (InviteObserver, bit);
			_options.set (ChangesDetected, true);
		}
		void SetCheckoutNotification (bool bit)
		{
			_options.set (CheckoutNotification, bit);
			_options.set (ChangesDetected, true);
		}
		void SetManualInvitationDispatch (bool bit)
		{
			_options.set (ManualInvitationDispatch, bit);
			_options.set (ChangesDetected, true);
		}
		void SetTransferHistory (bool bit)
		{
			_options.set (TransferHistory, bit);
			_options.set (ChangesDetected, true);
		}
		void SetMyComputer ()
		{
			_copyRequest.SetMyComputer ();
			_options.set (ChangesDetected, true);
		}
		void SetLAN ()
		{
			_copyRequest.SetLAN ();
			_options.set (ChangesDetected, true);
		}
		void SetInternet ()
		{
			_copyRequest.SetInternet ();
			_options.set (ChangesDetected, true);
		}

		void Clear ()
		{
			_userName.clear ();
			_emailAddress.clear ();
			_computerName.clear ();
			_copyRequest.Clear ();
			_options.init (0);
		}

		bool IsValid ();
		void DisplayErrors (Win::Dow::Handle owner) const;

	private:
		enum Options
		{
			InviteObserver,
			CheckoutNotification,
			InviteeOnSatellite,
			ManualInvitationDispatch,
			TransferHistory,
			ChangesDetected
		};

	private:
		// local data
		std::string const   _projectName;
		// invitee data
		std::string			_userName;
		std::string			_emailAddress;
		std::string			_computerName;
		// invitation handling
		FileCopyRequest		_copyRequest;
		BitSet<Options>		_options;
	};

	class OpenInvitationRequest : public OpenFileRequest
	{
	public:
		OpenInvitationRequest ();

		char const * GetFileFilter () const { return "Code Co-op Script File (*.snc)\0*.snc\0All Files (*.*)\0*.*"; }
	};

}

#endif
