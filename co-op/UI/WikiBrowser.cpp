//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"
#include "WikiBrowser.h"
#include "RecordSet.h"
#include "InPlaceBrowser.h"

WikiBrowser::~WikiBrowser () {}

void WikiBrowser::OnFocus ()
{
	_view.SetFocus ();
}

bool WikiBrowser::Show (FeedbackManager & feedback)
{
	if (_recordSet.get () == 0)
	{
		Restriction restriction;
		_recordSet = _tableProv.Query (_tableId, restriction);
		_recordSet->Attach (this);
		// row 0 contains project-unique temporary path
		_tmpDir = _recordSet->GetStringField (0, 0);
		// row 0 contains global wiki path
		_globalDir = _recordSet->GetStringField (1, 0);
		return true;
	}
	return false;
}

void WikiBrowser::UpdateIfNecessary (char const * topic)
{
	if (topic != 0 && strcmp (topic, "url") == 0)
	{
		_caption = _tableProv.QueryCaption (_tableId, _restriction);
	}
	else
	{
		Notify ("browser");
	}
}

void WikiBrowser::Invalidate ()
{
	_recordSet.reset ();
}

void WikiBrowser::Clear (bool forGood)
{
	_recordSet.reset ();
}