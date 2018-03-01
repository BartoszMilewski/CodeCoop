#if !defined (FEEDBACKMAN_H)
#define FEEDBACKMAN_H
//----------------------------------
// (c) Reliable Software 1998 - 2007
//----------------------------------

#include <Ctrl/ProgressMeter.h>
#include <Graph/Cursor.h>

class FeedbackManager : public Progress::Meter
{
public:
	Cursor::Handle GetBusyCursor () const { return _hourglass; }
	virtual void SetSupplementalActivity (char const * activity) {}
	virtual std::string GetCurrentActivity () { return std::string (); }

private:
	Cursor::Hourglass	_hourglass;
};

class SimpleMeter: public Progress::Meter
{
public:
	SimpleMeter (FeedbackManager * feedback)
		: _feedback (feedback)
	{}
	~SimpleMeter () 
	{
		_feedback->Close ();
	}
	void SetRange (int mini, int maxi, int step = 1)
	{
		_feedback->SetRange (mini, maxi, step);
	}
	void GetRange (int & min, int & max, int & step)
	{
		_feedback->GetRange (min, max, step);
	}
	void StepIt () { _feedback->StepIt (); }
	void StepTo (int step) { _feedback->StepTo (step); }
	void StepAndCheck () throw (Win::Exception) { _feedback->StepIt (); }
	void Close ()
	{
		_feedback->Close ();
	}
	void SetActivity (std::string const & activity) { _feedback->SetActivity (activity); }

private:
	FeedbackManager * _feedback;
};

class BusyIndicator
{
public:
	BusyIndicator (FeedbackManager * feedback)
		: _feedback (feedback),
		  _working (feedback->GetBusyCursor ()),
		  _prevActivity (feedback->GetCurrentActivity ())
	{
		_feedback->SetActivity ("Busy");
	}
	~BusyIndicator ()
	{
		if (_prevActivity.empty () || _prevActivity == "Ready")
			_feedback->Close ();
		else
			_feedback->SetActivity (_prevActivity.c_str ());
	}

private:
	FeedbackManager *	_feedback;
	Cursor::Holder		_working;
	std::string			_prevActivity;
};

class StatusTextSwitch
{
public:
	StatusTextSwitch (FeedbackManager & feedback, std::string const & newText)
		: _feedback (feedback)
	{
		_oldText = _feedback.GetCurrentActivity ();
		_feedback.SetActivity (newText.c_str ());
	}
	~StatusTextSwitch ()
	{
		_feedback.SetActivity (_oldText.c_str ());
	}

private:
	FeedbackManager &	_feedback;
	std::string			_oldText;
};

#endif
