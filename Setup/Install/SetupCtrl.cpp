//------------------------------------
//  (c) Reliable Software, 1996 - 2007
//------------------------------------

#include "precompiled.h"
#include "SetupCtrl.h"
#include "OutputSink.h"
#include "Global.h"
#include "BuildOptions.h"
#include "GlobalMessages.h"
#include "Catalog.h"
#include "License.h"
#include "resource.h"

#include <Win/Message.h>
#include <Ctrl/ProgressDialog.h>
#include <Graph/Canvas.h>
#include <Graph/CanvTools.h>
#include <Ex/WinEx.h>

SetupController::SetupController (Win::MessagePrepro & msgPrepro, InstallMode mode)
	: _timer (TimerId),
	  _msgPrepro (msgPrepro),
	  _mode (mode),
	  _bgColor (0xff, 0xff, 0xff),
	  _hiColor (_bgColor.ToNative () + 0x303030),
	  _loColor (_bgColor.ToNative () - 0x202020),
	  _brush (_bgColor)
{
	_productName = COOP_PRODUCT_NAME;
	_productName += " v. ";
	_productName += COOP_PRODUCT_VERSION;
	_setupCaption = _productName;
	_setupCaption += " Setup";
}

SetupController::~SetupController ()
{}

bool SetupController::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
    try
    {
		_timer.Attach (_h);
		// Font
		Font::Maker maker (24, "Verdana");
		maker.MakeHeavy ();
		_font = maker.Create ();
		Win::UpdateCanvas canvas (_h);
		Font::Holder font (canvas, _font);
		font.GetAveCharSize (_cxChar, _cyChar);

		_bitmap.Load (_h.GetInstance (), IDB_BITMAP);

		Win::UserMessage msg (UM_START_INSTALLATION);
		_h.PostMsg (msg);
		success = true;
    }
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
		success = false;
	}
	catch ( ... )
	{
		Win::ClearError ();
		TheOutput.Display ("Initialization -- Unknown Error", Out::Error); 
		success = false;
	}
	TheOutput.SetParent (_h);
	return true;
}

bool SetupController::OnDestroy () throw ()
{
	Win::Quit ();
	return true;
}

bool SetupController::OnSize (int width, int height, int flag) throw ()
{
	_cx = width;
	_cy = height;
	return true;
}
bool SetupController::OnEraseBkgnd(Win::Canvas canvas) throw()
{
	canvas.FillRect (Win::Rect (0, 0, _cx, _cy), _brush.ToNative ());
	return true;
}


bool SetupController::OnPaint () throw ()
{
	if (_mode != Full)
		return false;

	// full install
	Win::PaintCanvas canvas (_h);

	Bitmap::Logical logBmap (_bitmap);
	int w = logBmap.Width ();
	int h = logBmap.Height ();

	canvas.PaintBitmap (_bitmap, w, h);

	Win::TransparentBkgr trans (canvas);
	Font::Holder font (canvas, _font);
	font.GetAveCharSize (_cxChar, _cyChar);

	static char logo [] = "Reliable Software";
	static int lenLogo = strlen (logo);

	int x = 14;
	int y = 14;
	int xShift = 2;
	int yShift = 2;
	int dySecondLine = _cyChar + 10;
#if 0
	{
		Font::ColorHolder txtColor (canvas, _hiColor);
		canvas.Text (x + xShift * 2, y + yShift * 2, logo, lenLogo);
		canvas.Text (x + xShift * 2, y + yShift * 2 + dySecondLine, product.c_str (), product.length ());
	}
	{
		Font::ColorHolder txtColor (canvas, _loColor);
		canvas.Text (x, y, logo, lenLogo);
		canvas.Text (x, y + dySecondLine, product.c_str (), product.length ());
	}
#endif
	{
		Font::ColorHolder txtColor (canvas, _bgColor);
		canvas.Text (x + xShift, y + yShift, logo, lenLogo);
		canvas.Text (x + xShift, y + yShift + dySecondLine, _productName.c_str (), _productName.length ());
	}
	return true;
}

bool SetupController::OnTimer (int id) throw ()
{
	_timer.Kill ();
	// External tool timeout
	_installer->ConfirmInstallation ();
	_h.PostMsg (WM_CLOSE);
	return true;
}

bool SetupController::OnUserMessage (Win::UserMessage & msg) throw ()
{
	if (msg.GetMsg () == UM_START_INSTALLATION)
	{
		StartInstallation ();
		return true;
	}
	else if (msg.GetMsg () == UM_PROGRESS_TICK)
	{
		Progress::Meter & meter = _meterDialog->GetProgressMeter ();
		meter.StepIt ();
	}
	else if (msg.GetMsg () == UM_TOOL_END)
	{
		_timer.Kill ();
		_meterDialog.reset ();
		_installer->ConfirmInstallation ();
		_h.PostMsg (WM_CLOSE);
	}
	return false;
}

void SetupController::StartInstallation ()
{
	_installer.reset (new Installer (_msgPrepro, _h));
	try
	{
		switch (_mode)
		{
		case Full:
		{
			_meterDialog.reset (new Progress::MeterDialog (_setupCaption,
														   _h,
														   _msgPrepro,
														   false,	// Cannot cancel
														   0));		// Show immediately
			std::string caption ("Installing ");
			caption += _setupCaption;
			_meterDialog->SetCaption (caption);
			if (_installer->PerformInstallation (_meterDialog->GetProgressMeter ()))
			{
				Catalog catalog;
				License globalLicense (catalog);
				if (!globalLicense.IsCurrentVersion ())
				{
					// We have to select the global license
					unsigned toolTimeout = _installer->SelectGlobalLicense (_meterDialog->GetProgressMeter ());
					if (toolTimeout != 0)
					{
						_timer.Set (toolTimeout);
						return;
					}
				}

				// The user already has the global license
				_meterDialog.reset ();
				_installer->ConfirmInstallation ();
			}
			break;
		}
		case TemporaryUpdate:
			if (_installer->TemporaryUpdate ())
			{
				TheOutput.Display ("Your Code Co-op installation\n"
								   "has been updated successfully.\n\n"
								   "To restore the original version select\n"
								   "\"Restore Original Version\"command\n"
								   "from Co-op Help menu.");
			}
			break;
		case PermanentUpdate:
			if (_installer->PermanentUpdate ())
				TheOutput.Display ("Your Code Co-op installation\n"
								   "has been updated successfully.");
			break;
		case CmdLineTools:
			if (_installer->InstallCmdLineTools ())
				TheOutput.Display ("Code Co-op Command-line Tools\n"
								   "have been installed successfully.\n\n"
								   "Note: You may want to add the tools directory to\n"
								   "your PATH environment variable. You can either make\n"
								   "this change system-wide or make use of the CoopVars.bat\n"
								   "file provided with the command line tools.");
			break;
		};
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Installation -- Unknown Error", Out::Error); 
	}
	_h.PostMsg (WM_CLOSE);
}
