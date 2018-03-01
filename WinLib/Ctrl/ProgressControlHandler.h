#if !defined (PROGRESSCONTROLHANDLER_H)
#define PROGRESSCONTROLHANDLER_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <Win/Dialog.h>
#include <Ctrl/Button.h>
#include <Sys/Timer.h>

namespace Progress
{
	class MeterDialogData;

	class CtrlHandler: public ::Dialog::ControlHandler
	{
	public:
		enum
		{
			TimerId = 3105,
			Tick = 500		// 0,5 second
		};

	public:
		CtrlHandler (Progress::MeterDialogData & data, int dlgId);
		~CtrlHandler ();

		void Show ();
		virtual void Refresh () = 0;

		void OnTimer ();

	protected:
		Win::Button			_cancel;
		Win::Timer			_progressTimer;
		bool				_isVisible;

		Progress::MeterDialogData &	_dlgData;
	};
}

#endif
