//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "AccelTab.h"

#include <Win/Keyboard.h>

namespace Accel
{
	const Accel::Item Keys [] =
	{
		{Plain,				VKey::F5,		"View_Refresh"},		// Refresh
		{WithCtrl,			VKey::I,		"All_CheckIn"},			// Check-In All
		{WithCtrl,			VKey::Y,		"All_AcceptSynch"},		// Accept Synch
		{WithCtrl,			VKey::A,		"All_Select"},			// All Select
		{WithCtrl,			VKey::O,		"Selection_CheckOut"},  // Check-Out
		{WithCtrl,			VKey::X,		"Selection_Cut"},       // Cut
		{WithCtrl,			VKey::C,		"Selection_Copy"},      // Copy
		{WithCtrl,			VKey::V,		"Selection_Paste"},     // Paste
		{WithCtrl,			VKey::Return,	"Selection_OpenWithShell"},// Open with Shell
		{Plain,				VKey::Delete,	"Selection_Delete"},	// Delete
		{WithCtrl,			VKey::S,		"All_Synch"},			// All Synch Script
		{WithCtrl,			VKey::N,		"Selection_Synch"},		// Synch Next Script
		{Plain,				VKey::BackSpace,"GoBack"},				// Go Up or Go Back
		{Plain,				VKey::F1,		"Help_Contents"},		// Help
		{Plain,				VKey::F2,		"Selection_Rename"},	// File Rename
		{WithCtrl,			VKey::Tab,		"View_Next"},			// Switch view tabs
		{WithCtrlAndShift,	VKey::Tab,		"View_Previous"},		// Switch view tabs
		{WithAlt,			VKey::Return,	"Selection_Properties"},// Show item properties
		{ 0, 0, 0 }
	};
}
