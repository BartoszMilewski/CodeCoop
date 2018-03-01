#if !defined (UNKNOWNRECIPWIZ_H)
#define UNKNOWNRECIPWIZ_H
// ----------------------------------
// (c) Reliable Software, 2000 - 2002
// ----------------------------------

#include "resource.h"
#include "Address.h"
#include "Transport.h"
#include "WizardHelp.h"
#include "ConfigData.h"
#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/ComboBox.h>
#include <Ctrl/Button.h>
#include <Ctrl/Static.h>
#include <File/Path.h>
#include <Ctrl/PropertySheet.h>
#include <Ctrl/ListBox.h>

namespace UnknownRecipient
{
	// common data for all Unknown Recipient Wizard pages
	class Data
	{
		// revisit: get rid of these friends!
		friend class StandaloneCtrl;
		friend class Standalone2LanCtrl;
		friend class ProjectNameCtrl;
		friend class IntroCtrl;
		friend class HubOrSatCtrl;
		friend class PathCtrl;
		friend class UnknownHubIdCtrl;
		friend class ConfigErrCtrl;
		friend class UseEmailCtrl;
		friend class AddresseeCtrl;
	public:
		Data (Address const & address,
						std::set<Transport> const & transports,
						NocaseSet const & projectList,
						std::string const & comment,
						bool isHubIdKnown,
						bool isJoinRequest,
						bool isIncoming,
						bool isSenderLocal,
						ConfigData const & config)
			: _address (address),
			_comment (comment),
			_transports (transports),
			_projectList (projectList),
			_isHubIdKnown (isHubIdKnown),
			_isJoinRequest (isJoinRequest),
			_isIncoming (isIncoming),
			_isSenderLocal (isSenderLocal),
			_wantToBeSat (false),
			_answer (Unknown),
			_config (config)
		{}

		bool IsIncoming () const { return _isIncoming; }
		bool IsJoinRequest () const { return _isJoinRequest; }
		bool AreWeStandalone () const { return _config.GetTopology ().IsStandalone (); }
		bool DoWeUseEmail () const { return _config.GetTopology ().UsesEmail (); }
		bool CanIgnoreRecipient () const { return Ignore  == _answer; }
		bool IsCluster () const { return Cluster == _answer; }
		bool IsRemote () const  { return Remote  == _answer; }
		bool WantToStartUsingEmail () const { return UseEmail == _answer; }
		bool WantToBeSatellite () const { return _wantToBeSat; }
		Address const & GetAddress () const { return _address; }
		Transport const & GetTransport () const { return _transport; }
		std::set<Transport> const & GetTransports () const { return _transports; }
		NocaseSet const & GetProjectList () const { return _projectList; }
		void InitTransport (Transport const & transport)
		{
			_transport = transport;
		}
		void SetInterClusterTransportToMe (Transport const & transport)
		{
			_remoteTransport = transport;
			std::string const & hubId = _config.GetHubId ();
			if (hubId.empty () || IsNocaseEqual (hubId, "Unknown"))
				_hubId = transport.GetRoute ();
			else
				_hubId = hubId;
		}
		std::string const & GetHubId () const { return _hubId; }
		Transport const & GetInterClusterTransportToMe () const { return _remoteTransport; }

	private:
		bool const _isHubIdKnown;
		bool const _isJoinRequest;
		bool const _isIncoming;
		bool const _isSenderLocal;

		Address				_address;
		std::string const & _comment;

		enum
		{
			Unknown, // for initialisation only
			Ignore,
			Cluster,
			Remote,
			UseEmail
		} _answer;

		bool		_wantToBeSat; // valid only in standalone mode
		std::set<Transport> const & _transports; // known transports
		NocaseSet const & _projectList;
		Transport	_transport; // result
		Transport	_remoteTransport;
		std::string _hubId;
		ConfigData const & _config;
	};

	class TemplateCtrl : public PropPage::WizardHandler
	{
	public:
		TemplateCtrl (Data & data, unsigned pageId)
			: PropPage::WizardHandler (pageId, true), // supports context help
			  _dlgData (data)
		{}
		void OnHelp () const throw (Win::Exception) { OpenHelp (); }
	protected:
		bool GoNext (long & nextPage)
		{
			Assert (!"Must be overwritten");
			return false;
		}
		bool GoPrevious () { return true; }
	protected:
		Data & _dlgData;
	};

	class StandaloneCtrl : public TemplateCtrl
	{
	public:
		StandaloneCtrl (Data & data, unsigned pageId)
			: TemplateCtrl (data, pageId)
		{}
		bool OnInitDialog () throw (Win::Exception);
		bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	protected:
		bool GoNext (long & nextPage);
	private:
		Win::Edit     _project;
		Win::Edit     _hubId;
		Win::Edit	  _userId;
		Win::Edit	  _comment;

		Win::RadioButton _lan;
		Win::RadioButton _hubWithEmail;
		Win::RadioButton _mistake;
	};

	// Standalone to LAN

	class Standalone2LanCtrl : public TemplateCtrl
	{
	public:
		Standalone2LanCtrl (Data & data, unsigned pageId)
			: TemplateCtrl (data, pageId)
		{}
		bool OnInitDialog () throw (Win::Exception);
		bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
		void OnSetActive (long & result) throw (Win::Exception);
	protected:
		bool GoNext (long & nextPage);
	private:
		Win::Edit     _project;
		Win::Edit     _hubId;
		Win::Edit	  _userId;
		Win::Edit	  _comment;

		Win::RadioButton _hub;
		Win::RadioButton _sat;
		Win::RadioButton _mistake;
	};

	class ProjectNameCtrl : public TemplateCtrl
	{
	public:
		ProjectNameCtrl (Data & data, unsigned pageId)
			: TemplateCtrl (data, pageId)
		{}
		bool OnInitDialog () throw (Win::Exception);
		void OnSetActive (long & result) throw (Win::Exception);
	protected:
		bool GoNext (long & nextPage);
	private:
		Win::Edit     _name;
		Win::ListBox::Simple  _projectList;
		Win::Edit	  _comment;

		Win::RadioButton _misspelled;
		Win::RadioButton _correct;
	};

	class MisspelledCtrl : public TemplateCtrl
	{
	public:
		MisspelledCtrl (Data & data, unsigned pageId)
			: TemplateCtrl (data, pageId)
		{}
		void OnSetActive (long & result) throw (Win::Exception);
	};

	class AddresseeCtrl : public TemplateCtrl
	{
	public:
		AddresseeCtrl (Data & data, unsigned pageId)
			: TemplateCtrl (data, pageId)
		{}
		void OnSetActive (long & result) throw (Win::Exception);
	};

	class UnknownHubIdCtrl : public TemplateCtrl
	{
	public:
		UnknownHubIdCtrl (Data & data, unsigned pageId)
			: TemplateCtrl (data, pageId)
		{}
		bool OnInitDialog () throw (Win::Exception);
		void OnSetActive (long & result) throw (Win::Exception);
		bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	protected:
		bool GoNext (long & nextPage);
	private:
		Win::StaticText _descriptionText;
		Win::Edit     _name;
		Win::Edit     _hubId;
		Win::Edit	  _comment;

		Win::RadioButton _remote;
		Win::RadioButton _mistake;
	};

	class IntroCtrl : public TemplateCtrl
	{
	public:
		IntroCtrl (Data & data, unsigned pageId)
			: TemplateCtrl (data, pageId)
		{}
		bool OnInitDialog () throw (Win::Exception);
		bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
		void OnSetActive (long & result) throw (Win::Exception);
		void OnFinish (long & result) throw (Win::Exception);
	protected:
		bool GoNext (long & nextPage);
	private:
		Win::Edit     _name;
		Win::Edit     _hubId;
		Win::Edit     _userId;
		Win::Edit	  _comment;

		Win::RadioButton _ignore;
		Win::RadioButton _isCluster;
		Win::RadioButton _isRemote;
	};

	class HubOrSatCtrl : public TemplateCtrl
	{
	public:
		HubOrSatCtrl (Data & data, unsigned pageId)
			: TemplateCtrl (data, pageId)
		{}
		bool OnInitDialog () throw (Win::Exception);
		void OnSetActive (long & result) throw (Win::Exception);
	protected:
		bool GoNext (long & nextPage);
	private:
		Win::CheckBox	_isHub;
		Win::CheckBox	_isSat;
		Win::Edit		_hubId;
	};

	class PathCtrl : public TemplateCtrl
	{
	public:
		PathCtrl (Data & data, unsigned pageId)
			: TemplateCtrl (data, pageId)
		{}
		bool OnInitDialog () throw (Win::Exception);
		bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
		void OnSetActive (long & result) throw (Win::Exception);
		void OnFinish (long & result) throw (Win::Exception);
	private:
		Win::ComboBox	_pathCombo;
		Win::Button		_browse;
	};

	class ConfigErrCtrl : public TemplateCtrl
	{
	public:
		ConfigErrCtrl (Data & data, unsigned pageId)
			: TemplateCtrl (data, pageId)
		{}
		bool OnInitDialog () throw (Win::Exception);
		void OnSetActive (long & result) throw (Win::Exception);
		void OnFinish (long & result) throw (Win::Exception);
	private:
		Win::Edit     _hubIdEdit;
	};

	class UseEmailCtrl : public TemplateCtrl
	{
	public:
		UseEmailCtrl (Data & data, unsigned pageId)
			: TemplateCtrl (data, pageId)
		{}
		void OnSetActive (long & result) throw (Win::Exception);
		void OnFinish (long & result) throw (Win::Exception);
	};

	// -----------
	// The Wizards
	// -----------
	class Standalone2LanHandlerSet : public PropPage::HandlerSet
	{
	public:
		Standalone2LanHandlerSet (
			UnknownRecipient::Data & unknownRecipData, 
			bool isJoinRequest);

	private:
		Standalone2LanCtrl _standalone2LanCtrl;
		PathCtrl		   _pathCtrlHub;
		PathCtrl		   _pathCtrlSat;
	};

	class Standalone2LanOrEmailHandlerSet : public PropPage::HandlerSet
	{
	public:
		Standalone2LanOrEmailHandlerSet (
			UnknownRecipient::Data & unknownRecipData, 
			bool isJoinRequest);

	private:
		StandaloneCtrl		_standaloneCtrl;
		HubOrSatCtrl		_hubOrSatCtrl;
		PathCtrl		   _pathCtrlHub;
		PathCtrl		   _pathCtrlSat;
	};

	class DefectedMisspelledOrClusterHandlerSet : public PropPage::HandlerSet
	{
	public:
		DefectedMisspelledOrClusterHandlerSet (UnknownRecipient::Data & unknownRecipData);

	private:
		ProjectNameCtrl		_projectNameCtrl;
		MisspelledCtrl		_misspelledCtrl;
		AddresseeCtrl		_addresseeCtrl;
		IntroCtrl			_introCtrlLan;
		IntroCtrl			_introCtrl;
		PathCtrl			_pathCtrl;
		ConfigErrCtrl		_errCtrl;
	};

	class SwitchToEmailHandlerSet : public PropPage::HandlerSet
	{
	public:
		SwitchToEmailHandlerSet (UnknownRecipient::Data & unknownRecipData);

	private:
		UnknownHubIdCtrl _unknownHubIdCtrl;
		UseEmailCtrl     _useEmailCtrl;
	};

}

#endif
