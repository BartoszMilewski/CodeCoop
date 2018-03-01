#include "precompiled.h"
#include "OutSink.h"
#include "resource.h"
#include "MainCtrl.h"
#include "PathRegistry.h"
#include <Win/WinMain.h>
#include <Win/MsgLoop.h>

Out::Sink TheOutput;

int Win::Main (Win::Instance hInst, char const * cmdParam, int cmdShow)
{
    try
    {
		TheOutput.Init ("Code Co-op Licenser", Registry::GetLogsFolder ());
		Win::MessagePrepro msgPrepro;
		std::unique_ptr<MainController> ctrl (new MainController (msgPrepro));
		MainControlHandler handler (ID_MAIN, ctrl.get ());
		Dialog::ModelessMaker dlgMaker (handler, std::move(ctrl));
		dlgMaker.SetInstance (hInst);
		Dialog::Handle h = dlgMaker.Create (0);

		return msgPrepro.Pump ();
    }
    catch (Win::Exception e)
    {
        TheOutput.Display (e);
    }
    catch (...)
    {
		Win::ClearError ();
        TheOutput.Display ("Internal error: Unknown exception", Out::Error);
    }
	return 0;
}