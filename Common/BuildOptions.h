#if !defined (BUILDOPTIONS_H)
#define BUILDOPTIONS_H
//----------------------------------
// (c) Reliable Software 2000 - 2010
//----------------------------------

#if defined (COOP_WIKI)
#define COOP_PRODUCT_VERSION			"5.2"
#define COOP_PRODUCT_VERSION_NUMBER		5,2,0,0
#define COOP_PRODUCT_NAME				"Club Wiki"
#define DISPATCHER_PRODUCT_NAME			"Dispatcher Lite"
#elif defined (COOP_LITE)
#define COOP_PRODUCT_VERSION			"5.2a"
#define COOP_PRODUCT_VERSION_NUMBER		5,2,0,0
#define COOP_PRODUCT_NAME				"Code Co-op Lite"
#define DISPATCHER_PRODUCT_NAME			"Dispatcher Lite"
#else
#define COOP_PRODUCT_VERSION			"5.2a"
#define COOP_PRODUCT_VERSION_NUMBER		5,2,0,0
#define COOP_PRODUCT_NAME				"Code Co-op"
#define DISPATCHER_PRODUCT_NAME			"Dispatcher"
#endif

#define COPYRIGHT						"Copyright © Reliable Software 1996 - 2011"

#define	COOP_FILE_VERSION				COOP_PRODUCT_VERSION
#define COOP_FILE_VERSION_NUMBER		COOP_PRODUCT_VERSION_NUMBER
#define DISPATCHER_FILE_VERSION			COOP_PRODUCT_VERSION
#define DISPATCHER_FILE_VERSION_NUMBER	COOP_PRODUCT_VERSION_NUMBER
#define DIFFER_FILE_VERSION				COOP_PRODUCT_VERSION
#define DIFFER_FILE_VERSION_NUMBER		COOP_PRODUCT_VERSION_NUMBER

#endif
