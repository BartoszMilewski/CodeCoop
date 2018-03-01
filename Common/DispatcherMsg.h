#if !defined (DISPATCHERMSG_H)
#define DISPATCHERMSG_H
//----------------------------------
// (c) Reliable Software 1998 - 2008
// ---------------------------------

// user messages
unsigned const UM_TASKBAR_ICON_NOTIFY	= 1;
unsigned const UM_FINISH_SCRIPT			= 2;
unsigned const UM_REFRESH_VIEW			= 3;
unsigned const UM_SCRIPT_STATUS_CHANGE	= 4;
unsigned const UM_PROXY_CONNECTED 	    = 5; // wParam: 0 - disconnected, other - connected
unsigned const UM_CONFIG_WIZARD         = 6;

#endif
