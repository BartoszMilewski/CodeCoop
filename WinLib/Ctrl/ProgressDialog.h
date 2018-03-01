#if !defined (PROGRESSDIALOG_H)
#define PROGRESSDIALOG_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <Ctrl/ProgressDialogData.h>
#include <Ctrl/ProgressControlHandler.h>
#include <Ctrl/ProgressDialogController.h>
#include <Ctrl/ProgressChannel.h>
#include <Ctrl/Static.h>
#include <Ctrl/ProgressBar.h>
#include <Win/Dialog.h>
#include <Win/DialogTemplate.h>
#include <Sys/Synchro.h>
#include <Bit.h>

namespace Win
{
	class MessagePrepro;
}

namespace Progress
{
	class Dialog
	{
	public:
		Dialog (std::string const & title,
				Win::Dow::Handle parent, 
				Win::MessagePrepro & msgPrepro,
				unsigned initialDelay,	// Wait before showing progress meter dialog
				unsigned dlgId)
			: _dlgData (title, initialDelay),
			  _parentWin (parent),
			  _msgPrepro (msgPrepro),
			  _dlgId (dlgId)
		{}
		virtual ~Dialog ()
		{}

		void SetCaption (std::string const & caption) { _dlgData.SetCaption (caption); }
		void OnSetRange ();
		void PumpDialog ();
		void Close ();

		virtual Progress::Meter & GetProgressMeter () = 0;
		virtual Progress::Meter & GetOverallMeter () = 0;
		virtual Progress::Meter & GetSpecificMeter () = 0;
		virtual void CreateDialogTemplate (::Dialog::Template & tmpl) = 0;
		virtual Progress::CtrlHandler * GetControlHandler () { return 0; }

	protected:
		Progress::MeterDialogData	_dlgData;

	private:
		Win::Dow::Handle			_parentWin;
		Win::MessagePrepro &		_msgPrepro;
		::Dialog::Handle			_dlgHandle;
		unsigned					_dlgId;
	};

	class BlindDialog : public Progress::Dialog
	{
	public:
		BlindDialog (Win::MessagePrepro & msgPrepro);

		Progress::Meter & GetProgressMeter () { return *_blindMeter; }
		Progress::Meter & GetOverallMeter () { return *_blindMeter; }
		Progress::Meter & GetSpecificMeter () { return *_blindMeter; }
		void CreateDialogTemplate (::Dialog::Template & tmpl) {}

	private:
		std::unique_ptr<Progress::Meter>	_blindMeter;
	};

	class SingleCtrlHandler : public Progress::CtrlHandler
	{
	public:
		SingleCtrlHandler (Progress::MeterDialogData & data, Progress::Channel & channel);

		void Refresh ();

		bool OnInitDialog () throw (Win::Exception);
		bool OnCancel () throw ();

	private:
		Win::StaticText		_caption;
		Win::StaticText		_activity;
		Win::ProgressBar	_bar;
		std::string			_activityStr;
		Progress::Channel &	_channel;
	};

	class MeterDialog : public Progress::Dialog
	{
	public:
		enum Ids
		{
			DlgId = 256,
			CaptionId,
			ActivityId,
			ProgressBarId
		};

	public:
		MeterDialog (std::string const & title,
					 Win::Dow::Handle parent, 
					 Win::MessagePrepro & msgPrepro,
					 bool canCancel = true,
					 unsigned initialDelay = 1000)	// Wait 1 s before showing progress meter dialog
			: Progress::Dialog (title, parent, msgPrepro, initialDelay, DlgId),
			  _channel (canCancel),
			  _ctrlHandler (new Progress::SingleCtrlHandler (_dlgData, _channel)),
#pragma warning(disable: 4355) // 'this' used before initialized
			  _channelHandler (_channel, *this)
#pragma warning(default: 4355)
		{}

		~MeterDialog ()
		{
			Close ();
		}

		Progress::Meter & GetProgressMeter () { return _channelHandler; }
		Progress::Meter & GetOverallMeter () { return _channelHandler; }
		Progress::Meter & GetSpecificMeter () { return _channelHandler; }
		void CreateDialogTemplate (::Dialog::Template & tmpl);
		Progress::CtrlHandler * GetControlHandler () { return _ctrlHandler.get (); }

	private:
		Progress::Channel							_channel;
		std::unique_ptr<Progress::SingleCtrlHandler>	_ctrlHandler;
		Progress::ChannelHandler					_channelHandler;
	};
}

#endif
