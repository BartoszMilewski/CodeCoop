#if !defined (EDITPARAMS_H)
#define EDITPARAMS_H
//-----------------------------------
// (c) Reliable Software, 1997 - 2006
//-----------------------------------

#include <Ctrl/Messages.h>

// User messages

// wParam = SB_VERT, SB_HORZ or SB_CTL
const unsigned UM_GETDOCDIMENSION	= 4;
const unsigned UM_GETWINDIMENSION	= 5;
// wParam = SB_VERT, SB_HORZ or SB_CTL, lParam = new position
const unsigned UM_SETDOCPOSITION	= 6;
// wParam = SB_VERT, SB_HORZ, return value = doc position
const unsigned UM_GETDOCPOSITION	= 7;

// lParam = EditBuf*
const unsigned UM_INITDOC			= 8;

const unsigned UM_GIVEFOCUS			= 9;
const unsigned UM_CHANGE_FONT		= 10;

const unsigned UM_SEARCH_NEXT		= 11;
const unsigned UM_SEARCH_PREV		= 12;
const unsigned UM_EDIT_COPY			= 13;
const unsigned UM_EDIT_PASTE		= 14;
const unsigned UM_EDIT_CUT			= 15;
const unsigned UM_EDIT_DELETE		= 16;
// lParam = size* . Return value 1  or 0. If return 0 then file is saved, and size is not set
// For "force" seting size use wParam = 1
const unsigned UM_EDIT_NEEDS_SAVE	= 17;
const unsigned UM_EDIT_IS_READONLY	= 18;
// lParam = char * buf. wParam = 0 - buf will be saved; wParam = 1 - otherwise. Return value = size file to save
const unsigned UM_GET_BUF			= 19;
const unsigned UM_SCROLL_NOTIFY		= 20;
const unsigned UM_SYNCH_SCROLL		= 21;
const unsigned UM_REFRESH_UI		= 23;
const unsigned UM_DOCPOS_UPDATE		= 24;
const unsigned UM_SELECT_ALL		= 25;
// Returns 1 if file has been checked-out; 0 otherwise
const unsigned UM_CHECK_OUT			= 26;
// wParam = 0 - writable; wParam = 1 - readonly; lParam not used
const unsigned UM_SET_EDIT_READONLY = 27;

// wParam not used, lParam = DlgData * or not used 
const unsigned UM_GET_SELECTION = 28;
const unsigned UM_FIND_NEXT = 29;
// wParam = lineBreakingOn, 
const unsigned UM_TOGGLE_LINE_BREAKING = 30;
// wParam = SB_VERT or SB_HORZ , in  lParam return  new position
const unsigned UM_GETSCROLLPOSITION	= 31;
// wParam, lParam no used
const unsigned UM_CLEAR_EDIT_STATE = 32;
// wParam, lParam no used
const unsigned UM_UPDATE_PLACEMENT  = 33;
// wParam not used, lParam = ReplaceRequest * 
const unsigned UM_REPLACE           = 34;
const unsigned UM_REPLACE_ALL       = 35;
// wParam, lParam no used
const unsigned UM_INIT_DLG_REPLACE  = 36;
// wParam, lParam not used
const unsigned UM_GET_DOC_SIZE      = 37;
// wParam not used, lParam - paraNo
const unsigned UM_GOTO_PARAGRAPH    = 38;

const unsigned UM_UPDATESCROLLBARS	= 39;
// wParam - scroll bar; lParam - New 32-bit scroll position
const unsigned UM_SETSCROLLBAR		= 40;
// wParam = 0 - don't hide any, 1 - hide left, 2 - hide right
const unsigned UM_HIDE_WINDOW		= 41;
const unsigned UM_EXIT				= 42;
// Returns 1 if undo possible, 0 otherwise
const unsigned UM_CAN_UNDO			= 43;
// wParam not used, lParam - number action
const unsigned UM_UNDO				= 44;
// Returns 1 if undo possible, 0 otherwise
const unsigned UM_CAN_REDO			= 45;
// wParam not used, lParam - number action
const unsigned UM_REDO				= 46;
// wParam, lParam not used
const unsigned UM_PREPARE_CHANGE_NUMBER_PANES = 47;
// lParam = window handle
const unsigned UM_SET_CURRENT_EDIT_WIN = 48;

#endif
