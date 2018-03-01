#if !defined DIFFMAIN_H
#define DIFFMAIN_H
//------------------------------------
//  (c) Reliable Software, 1996, 97
//------------------------------------

#include "CommandParams.h"
#include <Win/Controller.h>

LRESULT CALLBACK WndProcMargin
   (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

using CmdLine::CommandParams;

extern CmdLine::CommandParams * TheParams;

#endif

