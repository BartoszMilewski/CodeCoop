// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include "precompiled.h"
#include "Pop3InboxIterator.h"
#include "OutputSink.h"
#include "Registry.h"
#include <Mail/MsgParser.h>
#include <Mail/Pop3Message.h>
#include <File/Path.h>

// used only in script logging
#include <File/SafePaths.h>

Pop3InboxIterator::Pop3InboxIterator (Pop3::Session & session, bool isDeleteNonCoopMsg)
	: _retriever (session),
	  _isDelNonCoopMsg (isDeleteNonCoopMsg)
{
	Registry::UserDispatcherPrefs prefs;
	_isLogging = prefs.IsEmailLogging ();

	RetrieveSubject ();
}

void Pop3InboxIterator::Advance ()
{
	Assert (!AtEnd ());
	_retriever.Advance ();
	RetrieveSubject ();
}

void Pop3InboxIterator::RetrieveSubject ()
{
	if (!_retriever.AtEnd ())
	{
		Pop3::Message msgHdr;
		Pop3::Parser parser;
		parser.Parse (_retriever.RetrieveHeader (), msgHdr);
		_subject = msgHdr.GetSubject ();
	}
}

void Pop3InboxIterator::RetrieveAttachements (SafePaths & attPaths)
{
	Assert (!AtEnd ());
	Assert (attPaths.IsEmpty ());
	Pop3::Message msg;
	Pop3::Parser parser;
	parser.Parse (_retriever.RetrieveMessage (), msg);
	TmpPath tmpFolder;
	msg.SaveAttachments (tmpFolder, attPaths);
	if (_isLogging)
	{
		// save script attachments for debugging purposes
		for (SafePaths::iterator it = attPaths.begin (); it != attPaths.end (); ++it)
		{
			PathSplitter attPath (*it);
			std::string attFilename (_subject);
			attFilename += attPath.GetExtension ();
			TheOutput.LogFile (*it, attFilename, "Pop3 Script Log");
		}
		// save script note for debugging purposes
		std::string scriptNote (msg.GetSubject ());
		scriptNote += '\n';
		scriptNote += msg.GetText ();
		TheOutput.LogNote ("Pop3Scripts.log", scriptNote);
	}
}

// return true if client can continue iteration
bool Pop3InboxIterator::DeleteMessage () throw ()
{
	try
	{
		Assert (!AtEnd ());
		_retriever.DeleteMsgFromServer ();
		return true;
	} catch (...) {}
	return false;
}

void Pop3InboxIterator::CleanupMessage () throw ()
{
	try
	{
		_retriever.SkipToEnd ();
		Assert (!AtEnd ());
		_retriever.DeleteMsgFromServer ();
	} catch (...) {}
}