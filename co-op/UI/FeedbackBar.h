#if !defined (FEEDBACKBAR_H)
#define FEEDBACKBAR_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2007
//------------------------------------

#include "FeedbackMan.h"

#include <Ctrl/StatusBar.h>
#include <Ctrl/ProgressBar.h>

class FeedbackBar : public FeedbackManager
{
public:
	FeedbackBar (Win::Dow::Handle win);

	int Height () const { return _statusBar.Height (); }
	void Size (Win::Rect const & feedbackRect);
	void SetFont (Font::Descriptor const & font)
	{
		_statusBar.SetFont (font);
	}

	// Progress Meter
	void SetRange (int mini, int maxi, int step)
	{
		_progressBar.SetRange (mini, maxi, step);
		_progressBar.Show ();
	}
	void StepIt () { _progressBar.StepIt (); }
	void StepTo (int step) { _progressBar.StepTo (step); }
	void StepAndCheck () throw (Win::Exception) { _progressBar.StepIt (); }
	void Close ()
	{
		_progressBar.Hide ();
		SetSupplementalActivity ("");
		SetActivity ("Ready");
	}
	void SetActivity (std::string const & activity) { _statusBar.SetText (activity.c_str ()); }

	// Feedback Manager
	void SetSupplementalActivity (char const *text) { _statusBar.SetText (text, 2); }
	std::string GetCurrentActivity ();

private:
	Win::StatusBar		_statusBar;
	Win::ProgressBar	_progressBar;
};

#endif
