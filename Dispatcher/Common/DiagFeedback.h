#if !defined (DIAGFEEDBACK_H)
#define DIAGFEEDBACK_H
//-------------------------------------
//  DiagFeedback.h
//  (c) Reliable Software, 1999 -- 2002
//-------------------------------------

class Edit;
class ProgressBar;
namespace Win 
{ 
	class MessagePrepro; 
	class Edit;
	class ProgressBar;
}

class DiagFeedback
{
public:
	DiagFeedback (Win::Edit & status)
		: _status (status)
	{}

	void Display (char const * info);
	void Newline ();
	void Clear ();

private:
	Win::Edit &	_status;
};

class DiagnosticsProgress
{
public:
	virtual ~DiagnosticsProgress () {}

	virtual void SetRange (int mini, int maxi, int step = 1) = 0;
	virtual void StepIt () = 0;
	virtual void Clear () = 0;
	virtual bool WasCanceled () = 0;
};

class NullDiagProgress : public DiagnosticsProgress
{
public:
	void SetRange (int mini, int maxi, int step = 1) {}
	void StepIt () {}
	virtual void Clear () {}
	bool WasCanceled () { return false; }
};

class DiagProgress : public DiagnosticsProgress
{
public:
	DiagProgress ()
		: _wasCanceled (false)
	{}

	void Init (Win::ProgressBar * progress, Win::MessagePrepro * msgPrepro);
	void SetRange (int mini, int maxi, int step = 1);
	void StepIt ();
	void Cancel () { _wasCanceled = true; }
	bool WasCanceled ();
	void Clear ();

private:
	Win::ProgressBar *		_progress;
	Win::MessagePrepro *	_msgPrepro;
	bool					_wasCanceled;
};

#endif
