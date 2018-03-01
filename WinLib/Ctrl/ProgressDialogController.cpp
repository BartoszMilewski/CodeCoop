//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "WinLibBase.h"
#include "ProgressDialogController.h"
#include "ProgressControlHandler.h"

Progress::DialogController::DialogController (Progress::CtrlHandler & handler,
											  Win::MessagePrepro & prepro,
											  int dlgId)
	: Dialog::ModelessController (prepro, dlgId),
	  _ctrlHandler (handler)
{}

bool Progress::DialogController::OnTimer (int id) throw (Win::Exception)
{
	if (id != Progress::CtrlHandler::TimerId)
		return false;

	_ctrlHandler.OnTimer ();
	return true;
}
