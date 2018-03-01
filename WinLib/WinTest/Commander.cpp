//----------------------------
// (c) Reliable Software, 2004
//----------------------------
#include "precompiled.h"
#include "Commander.h"
#include "TopCtrl.h"
#include "About.h"
#include "Modeless.h"
#include "ListviewDlg.h"
#include "resource.h"

#include <Win/DialogTemplate.h>

void Commander::Program_Exit () 
{
	_topWindow.Destroy ();
}


void Commander::Program_About ()
{
	AboutDlgHandler handler;

	Dialog::TemplateMaker::Button button (IDOK);
	button.SetRect (69,69,50,14);
	button.SetText (L"OK");
	button.Style () << Win::Button::Style::Default;

	Dialog::TemplateMaker::StaticText txt1;
	txt1.SetRect (7,52,172,14);
	txt1.SetText (L"(c) Reliable Software 2005");
	txt1.Center ();

	Dialog::TemplateMaker::Button buttonRs (IDB_RELISOFT);
	buttonRs.SetRect (43,31,99,16);
	buttonRs.SetText (L"Visit our web site!");

	Dialog::TemplateMaker::StaticText txt2;
	txt2.SetRect (7,14,172,16);
	txt2.SetText (L"For more free programs and tutorials");
	txt2.Center ();

	Dialog::TemplateMaker::Button buttonC (IDCANCEL);
	buttonC.SetRect (69,69,50,14);
	buttonC.SetText (L"Cancel");
	buttonC.Style ().SetInvisible ();

	Dialog::TemplateMaker::ProgressBar bar (125);
	bar.SetRect (7, 32, 172, 12);

	Dialog::TemplateMaker templateMaker;
	templateMaker.SetRect (Win::Rect (0, 0, 186, 90));
	templateMaker.SetTitle (L"WinTest");

	templateMaker.AddItem (&button);
	templateMaker.AddItem (&txt1);
	templateMaker.AddItem (&buttonRs);
	templateMaker.AddItem (&txt2);
	templateMaker.AddItem (&buttonC);
	//templateMaker.AddItem (&bar);

	Dialog::Template t1;
	templateMaker.Create (t1);
	// For comparison
	Dialog::Template t2;
	t2.Load (_topWindow.GetInstance (), IDD_ABOUT);

	Dialog::Modal (_topWindow,
			   _topWindow.GetInstance (), 
			   t1, 
			   handler);

	// Dialog::Modal dlg (_topWindow, handler);

}

void Commander::Dialogs_Modeless ()
{
	_modelessMan->Open (_topWindow);
}

void Commander::Dialogs_Listview ()
{
	std::string itemName; // the result (here it's discarded)

	// prepare data for the dialog
	TableData data;
	data.AddRow ("First Item", "working" );
	data.AddRow ("Second Item", "broken" );

	ListingCtrlHandler ctrl (data);
	Dialog::Modal dialog (_topWindow, ctrl);
	if (dialog.IsOK ())
		itemName = data.GetSelection ();
}
