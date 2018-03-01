#if !defined (MESSAGES_H)
#define MESSAGES_H
//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

// Reserved by Reliable Software Library
const unsigned int MSG_RS_LIBRARY		= 0x4000;

// wParam = splitter id
// lParam = new splitter position relative to the parent's left edge
const unsigned int UM_VSPLITTERMOVE		= MSG_RS_LIBRARY + 1;
// wParam = splitter id
// lParam = new splitter position relative to the parent's top edge
const unsigned int UM_HSPLITTERMOVE		= MSG_RS_LIBRARY + 2;

// Margin control selection: wParam, lParam from mouse messages
const unsigned int UM_STARTSELECTLINE	= MSG_RS_LIBRARY + 3;
const unsigned int UM_SELECTLINE		= MSG_RS_LIBRARY + 4;
const unsigned int UM_ENDSELECTLINE		= MSG_RS_LIBRARY + 5;

// Folder watcher sends this message to its notify sink window
// wParam = 0 - change report; 1 - folder is ready to be removed from the watch list
// lParam = char const * folder
const unsigned int UM_FOLDER_CHANGE		= MSG_RS_LIBRARY + 6;

// Inter process communication message
// wParam = registered message send
// lParam = handle to global memory buffer
const unsigned int UM_INTERPROCESS_PACKAGE = MSG_RS_LIBRARY + 7;

// Last message used by RS LIBRARY
const unsigned int UM_RS_LIBRARY_LAST	= MSG_RS_LIBRARY + 7;

#endif
