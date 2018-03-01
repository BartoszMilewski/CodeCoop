//------------------------------------
//  DiagFeedback.cpp
//  (c) Reliable Software, 1999
//------------------------------------

#include "precompiled.h"
#include "DiagFeedback.h"

#include <Ctrl/Edit.h>
#include <Ctrl/ProgressBar.h>
#include <Win/MsgLoop.h>

#include <LightString.h>

void DiagFeedback::Display (char const * info)
{
	Msg lastLine;
	lastLine << info << "\r\n";
	_status.Append (lastLine.c_str ());
}

void DiagFeedback::Newline ()
{
	_status.Append ("\r\n");
}

void DiagFeedback::Clear ()
{
	_status.SetReadonly (false);
	_status.Clear ();
	_status.SetReadonly (true);
	_status.Update (); // force immediate repainting
}

void DiagProgress::Init (Win::ProgressBar * progress, Win::MessagePrepro * msgPrepro)
{
	_progress = progress;
	_msgPrepro = msgPrepro;
}

void DiagProgress::SetRange (int mini, int maxi, int step)
{
	_progress->Enable ();
	_progress->SetRange (mini, maxi, step);
}

void DiagProgress::StepIt ()
{
	_progress->StepIt ();
}

bool DiagProgress::WasCanceled ()
{
	_msgPrepro->PumpDialog ();
	return _wasCanceled; 
}

void DiagProgress::Clear ()
{
	_progress->SetRange (0, 0, 0);
	_wasCanceled = false;
}
