#if !defined (PROGRESSDIALOGCONTROLLER_H)
#define PROGRESSDIALOGCONTROLLER_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <Win/Dialog.h>

namespace Win
{
	class MessagePrepro;
}

namespace Progress
{
	class CtrlHandler;

	class DialogController : public ::Dialog::ModelessController
	{
	public:
		DialogController (Progress::CtrlHandler & ctrlHandler,
						  Win::MessagePrepro & prepro,
						  int dlgId);

		bool OnTimer (int id) throw (Win::Exception);

	private:
		Progress::CtrlHandler & _ctrlHandler;
	};

}

#endif
