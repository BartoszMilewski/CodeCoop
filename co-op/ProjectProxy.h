#if !defined (PROJECTPROXY_H)
#define PROJECTPROXY_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "Model.h"
#include "FeedbackMan.h"

class Catalog;
class TransactionFileList;

namespace Project
{
	class Proxy
	{
	public:
		Proxy ()
			: _model (true)	// Quick visit
		{
			_model.SetUIFeedback (&_blindFeedback);
		}

		std::string const & GetProjectName () const { return _model.GetProjectName (); }
		char const * GetRootDir () const { return _model.GetProjectDir (); }
		Project::Db const & GetProjectDb () const { return _model.GetProjectDb (); }

		void Visit (int projectId, Catalog & catalog);
		void PreserveLocalEdits (TransactionFileList & fileList);
		void RequestVerification (UserId recipientId);
		void RestoreProject ();

	private:
		Model			_model;
		FeedbackManager	_blindFeedback;
	};
}

#endif
