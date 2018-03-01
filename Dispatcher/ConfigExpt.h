#if !defined (CONFIGEXPT_H)
#define CONFIGEXPT_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2002
// ----------------------------------

// Exception of class ConfigExpt is thrown 
// - after configuration is changed through Preferences dialog
// - whenever a user tells us to change configuration during 
//   mailbox processing run

// Handling ConfigExpt changes Dispatcher's configuration.
// As a result some data may need reconstructing (e.g. polimorfic Public Inboxes).
// Afterwards, mailbox processing run and/or e-mail check is forced.
// Catch the exception on a Controller level as only the Controller can set timers.

class ConfigExpt
{
public:
	ConfigExpt () {}
};

#endif
