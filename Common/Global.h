#if !defined (GLOBAL_H)
#define GLOBAL_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2007
//------------------------------------
//
// Products
//
char const coopProId = 'P';
char const coopLiteId = 'L';
char const clubWikiId = 'W';
//-------------- Current Product -------------------
#if defined (COOP_WIKI)
const char TheCurrentProductId = clubWikiId;
#elif defined (COOP_LITE)
const char TheCurrentProductId = coopLiteId;
#else
const char TheCurrentProductId = coopProId;
#endif
//-------------- Current Version -------------------
const long TheCurrentMajorVersion = 5;	// must be 0 - 15
//--------------------------------------------------

//--------------------------------------------------
//
// Windows class names
//
char const CoopClassName []			= "Reliable Software Code Co-op";
char const DispatcherClassName []	= "Reliable Software Code Co-op Mail Dispatcher";
char const DifferClassName []	    = "Reliable Software Code Co-op Differ";
char const ListWindowClassName []	= "Reliable Software Code Co-op List Window";
const char ServerClassName []		= "Reliable Software Code Co-op Server";
//
// Co-op global strings
//
char const ApplicationName []	= "Code Co-op"; // registry section name
char const DispatcherSection [] = "Dispatcher"; // registry section name
char const InBoxDirName []		= "InBox";
char const dbFormatName []		= "Database Format";
char const HelpFolder []		= "Help";		// only used by Install & Uninstall
char const TutorialFolder []	= "Tutorial";	// only used by Install & Uninstall
char const CurrentVersion []	= "Current project version";	// Fake entry in the history view
//
// Project members known names
//
char const AdminName []		= "Project Administrator";
char const UnknownName []	= "Unknown Project Member";
//
// Code Co-op server command line option names
//
char const Conversation []	= "conversation";
char const KeepAliveTimeout [] = "keepalivetimeout";
char const StayInProject [] = "stayinproject";
//
// Registered named messages
//
// Messages sent to Mail Dispatcher
// wParam - not used; lParam - not used
const char UM_COOP_PROJECT_CHANGE []= "UM_COOP_PROJECT_CHANGE";
// wParam != 0 -- user can join only as an observer; lParam - not used
const char UM_COOP_JOIN_REQUEST []	= "UM_COOP_JOIN_REQUEST";
// wParam = 0, for "Co-op Update" command; wParam = 1, for "Co-op Update Options" command;
// lParam - not used
const char UM_COOP_UPDATE [] = "UM_COOP_UPDATE";
// wParam -- notification size; lParam -- pointer to the notification buffer
const char UM_COOP_NOTIFICATION []	= "UM_COOP_NOTIFICATION";
// Inter-process conversation initiation
// wParam - global atom identyfying conversation name; lParam - not used
const char UM_IPC_INITIATE []		= "UM_IPC_INITIATE";
// Inter-process conversation deadlocked!
const char UM_IPC_ABORT []		= "UM_IPC_ABORT";
// Inter-process conversation termination
// wParam - not used; lParam - not used
const char UM_IPC_TERMINATE []		= "UM_IPC_TERMINATE";
// Display alert dialog, wParam -- timeout
const char UM_DISPLAY_ALERT []= "UM_DISPLAY_ALERT";
// Activity of WorkQueue
// wParam - not used; lParam - not used
const char UM_ACTIVITY_CHANGE []= "UM_ACTIVITY_CHANGE";
// wParam: event enumeration; lParam - not used
const char UM_DOWNLOAD_EVENT []= "UM_DOWNLOAD_EVENT";
// wParam - total bytes, lParam - transferred bytes
const char UM_DOWNLOAD_PROGRESS []= "UM_DOWNLOAD_PROGRESS";
// wParam - local project id; lParam -- 1 when new missing scripts detected
const char UM_COOP_PROJECT_STATE_CHANGE [] = "UM_COOP_PROJECT_STATE_CHANGE";
// wParam - new chunk size; lParam - not used
const char UM_COOP_CHUNK_SIZE_CHANGE [] = "UM_COOP_CHUNK_SIZE_CHANGE";
// wParam - not used; lParam - not used
const char UM_SHOW_WINDOW []= "UM_SHOW_WINDOW";
// wParam - not used; lParam - not used
const char UM_STARTUP []= "UM_STARTUP";
// wParam - not used; lParam - not used
const char UM_COOP_COMMAND [] = "UM_COOP_COMMAND";
// wParam - true - merge conflicst detected; lParam - pointer to the AutoMerger reporting completion
const char UM_AUTO_MERGER_COMPLETED [] = "UM_AUTO_MERGER_COMPLETED";
// wParam - not used; lParam - not used
const char UM_COOP_BACKUP [] = "UM_COOP_BACKUP";

int const trialDays						= 31;

#endif
