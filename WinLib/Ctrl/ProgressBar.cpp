//----------------------------------------------------
// ProgressBar.cpp
// (c) Reliable Software 2000
//
//----------------------------------------------------
#include <WinLibBase.h>
#include "ProgressBar.h"

using namespace Win;

ProgressBarMaker::ProgressBarMaker (Win::Dow::Handle hParent, int id)
	: Win::ControlMaker (PROGRESS_CLASS, hParent, id)
{}

void ProgressBar::SetRange (int min, int max, int step)
{
	SendMsg (PBM_SETRANGE, 0, MAKELPARAM (min, max));
	SendMsg (PBM_SETSTEP, step, 0);
	SendMsg (PBM_SETPOS, min, 0);
}

void ProgressBar::StepIt ()
{
	SendMsg (PBM_STEPIT, 0, 0);
}

void ProgressBar::StepTo (int min)
{
	SendMsg (PBM_SETPOS, min, 0);
}