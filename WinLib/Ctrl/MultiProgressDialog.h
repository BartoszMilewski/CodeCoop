#if !defined (MULTIPROGRESSDIALOG_H)
#define MULTIPROGRESSDIALOG_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <Ctrl/ProgressDialogData.h>
#include <Ctrl/ProgressDialog.h>
#include <Ctrl/ProgressControlHandler.h>
#include <Ctrl/ProgressDialogController.h>
#include <Ctrl/ProgressChannel.h>
#include <Ctrl/Static.h>
#include <Ctrl/ProgressBar.h>
#include <Win/DialogTemplate.h>

namespace Win
{
	class MessagePrepro;
}

namespace Progress
{
	class MultiCtrlHandler: public Progress::CtrlHandler
	{
	public:
		MultiCtrlHandler (Progress::MeterDialogData & data,
						  Progress::Channel & overall,
						  Progress::Channel & specific);

		void Refresh ();

		bool OnInitDialog () throw (Win::Exception);
		bool OnCancel () throw ();

	private:
		Win::StaticText		_caption;
		Win::StaticText		_overallActivity;
		Win::ProgressBar	_overallBar;
		Win::StaticText		_specificActivity;
		Win::ProgressBar	_specificBar;
		std::string			_overallActivityStr;
		std::string			_specificActivityStr;

		Progress::Channel &	_overall;
		Progress::Channel &	_specific;
	};

	class MultiMeterDialog : public Progress::Dialog
	{
	public:
		enum Ids
		{
			DlgId = 256,
			CaptionId,
			OverallActivityId,
			OverallProgressBarId,
			SpecificActivityId,
			SpecificProgressBarId
		};

	public:
		MultiMeterDialog (std::string const & title,
						  Win::Dow::Handle parent, 
						  Win::MessagePrepro & msgPrepro,
						  bool canCancel = true,
						  unsigned initialDelay = 1000)	// Wait 1 s before showing progress meter dialog
			: Progress::Dialog (title, parent, msgPrepro, initialDelay, DlgId),
			  _overall (canCancel),
			  _specific (false),
			  _ctrlHandler (new Progress::MultiCtrlHandler (_dlgData, _overall, _specific)),
#pragma warning(disable: 4355) // 'this' used before initialized
			  _overallMeter (_overall, *this),
			  _specificMeter (_specific, *this)
#pragma warning(default: 4355)
		{}
		~MultiMeterDialog ()
		{
			Close ();
		}

		Progress::Meter & GetProgressMeter () { return _overallMeter; }
		Progress::Meter & GetOverallMeter () { return _overallMeter; }
		Progress::Meter & GetSpecificMeter () { return _specificMeter; }
		void CreateDialogTemplate (::Dialog::Template & tmpl);
		Progress::CtrlHandler * GetControlHandler () { return _ctrlHandler.get (); }

	private:
		std::unique_ptr<Progress::MultiCtrlHandler>	_ctrlHandler;
		Progress::Channel							_overall;
		Progress::Channel							_specific;
		Progress::ChannelHandler					_overallMeter;
		Progress::ChannelHandler					_specificMeter;
	};
}

#endif
