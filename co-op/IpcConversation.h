#if !defined (IPCCONVERSATION_H)
#define IPCCONVERSATION_H
//-----------------------------------
//  (c) Reliable Software 2000 - 2008
//-----------------------------------

#include "CmdVector.h"
#include "IpcExchange.h"

#include <Sys/Active.h>

class Commander;
class InputSource;
class FeedbackUser;
class SelectionManager;

namespace CommandIpc
{
	class ConvStarter
	{
	public:
		virtual void OnFinishConv (bool stayInGui) = 0;
	};

	enum
	{
		ClientResponseTimeout = 5 * 1000
	};

	class Context
	{
	public:
		Context (Commander & commander, 
				 CmdVector const & cmdVector,
				 Win::CritSection & modelCritSect, 
				 FeedbackUser & feedbackUser)
			: _commander (commander),
			  _cmdVector (cmdVector),
			  _modelCritSect (modelCritSect),
			  _feedbackUser (feedbackUser)
		{}

		bool VisitProject (int projectId, Win::CritSection & critSect);
		Win::CritSection & GetModelCtricalSection () { return _modelCritSect; }
		// The X methods require that the model is locked
		FeedbackUser & XGetFeedbackManager () { return _feedbackUser; }
		Commander & XGetCommander () { return _commander; }
		SelectionManager & XGetSelectionManager ();
		bool XExecuteCommand (std::string const & command);

	private:
		Commander &			_commander;
		CmdVector const &	_cmdVector;
		Win::CritSection &	_modelCritSect; 
		FeedbackUser &		_feedbackUser; 
	};

	class ConversationData
	{
	public:
		ConversationData (std::string const & convName, bool quiet);
		bool IsValid() const { return !_clientEvent->IsNull(); }

		void WakeupClient ();
		void ChangeClientTimeout (unsigned int timeout);
		void MakeAck (std::string const & info = std::string ()) { _exchange.MakeAck (info); }
		void MakeErrorReport (std::string const & errMsg) { _exchange.MakeErrorReport (errMsg); }

		int GetProjectId () const { return _projectId; }
		char const * GetCmdStr ()
		{
			return reinterpret_cast<char const *>(_exchange.GetReadData ()); 
		}
		unsigned char const * GetData () { return _exchange.GetReadData (); }
		unsigned long GetDataSize () { return _exchange.GetBufSize (); }
		unsigned char * AllocBuf (unsigned int size) { return _exchange.AllocBuf (size); }

		bool IsCmd () const { return _exchange.IsCmd (); }
		bool IsLast () const { return _exchange.IsLast (); }
		bool IsDataRequest () const { return _exchange.IsDataRequest (); }
		bool IsStateRequest () const { return _exchange.IsStateRequest (); }
		bool IsVersionIdRequest () const { return _exchange.IsVersionIdRequest (); }
		bool IsReportRequest () const { return _exchange.IsReportRequest (); }
		bool IsCurrent () const { return _exchange.IsCurrent (); }
		bool IsForkIdsRequest () const { return _exchange.IsForkIdsRequest (); }
		bool IsTargetPathRequest () const { return _exchange.IsTargetPathRequest (); }
		bool IsDeepForkRequest () const { return _exchange.IsDeppForkRequest (); }

	private:
		IpcExchangeBuf				_exchange;
		int							_projectId;
		std::unique_ptr<Win::Event>	_clientEvent; // May be null!
	};

	class ServerConversation : public ActiveObject
	{
	public:
		ServerConversation (ConversationData * convData, Context & context, ConvStarter * starter)
			: _waitTimeout (ClientResponseTimeout),
			  _stayInGui (false),
			  _context (context),
			  _convData (convData),
			  _convStarter (starter)
		{
		}
		bool StayInGui () const { return _stayInGui; }
		int GetProjectId () const { return _convData->GetProjectId (); }

	protected:
		void Run ();
		void FlushThread () { _myEvent.Release (); }
		void Detach ();

	private:
		// These are called after entering the Model's critical section
		void ExecuteCmd ();
		void ReturnState ();
		void ReturnVersionId ();
		void ReturnReport ();
		void ReturnForkIds ();
		void ReturnTargetPath ();

	private:
		Win::CritSection	_critSect;
		ConversationData *	_convData;
		Context &			_context;
		ConvStarter *		_convStarter;

		Win::NewEvent		_myEvent;
		unsigned int		_waitTimeout;
		bool				_stayInGui;
	};

	class InputSourceConnector
	{
	public:
		InputSourceConnector (Commander & commander, InputSource * inputSource);
		~InputSourceConnector ();

	private:
		Commander &	_commander;
	};
}
#endif
