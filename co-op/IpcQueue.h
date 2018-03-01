#if !defined (IPCQUEUE_H)
#define IPCQUEUE_H
//-----------------------------------
//  (c) Reliable Software 2000 - 2007
//-----------------------------------
#include "CmdVector.h"
#include "IpcConversation.h"
#include <Sys/Active.h>
#include <auto_vector.h>

class Commander;
class FeedbackUser;

namespace CommandIpc
{
	enum
	{
		OneSecond = 1000,	// In milliseconds
		OneMinute = 60 * OneSecond,
		VisitProjectTimeout = 5 * OneSecond,
		OneConversationTimeout = 3 * OneSecond,
		IpcTimeout = 5 * OneSecond,
		LongCommandTimeout = 60 * OneMinute
	};

	// revisit: totally wrong implementation
	class RequestQueue
	{
	public:
		void Add (std::unique_ptr<ConversationData> request);
		ConversationData * GetRequest ();
		void Clear ();

		bool IsEmpty () const { return _queue.empty (); }

	private:
		auto_vector<ConversationData>	_requests;
		std::deque<unsigned int>		_queue;
	};

	//----------------------------------------------
	// The queue of conversations started by clients
	//----------------------------------------------
	class Queue: public ActiveObject, public ConvStarter
	{
		friend class RequestQueue;

	public:
		Queue (Win::Dow::Handle parentWin, Context & context);

		void InitiateConv (std::string const & convName);
		void OnFinishConv (bool stayInGui);
		bool StayInGui () const { return _stayInGui; }
		int  GetProjectId () const { return _lastProjectId; }
		bool IsIdle ();

	private:
		bool IsEmpty () const { return _activeConv.empty (); }
		void Run ();
		void FlushThread () { _event.Release (); }

	private:
		bool NextConversation ();

	private:
		Win::Dow::Handle	_parentWin;
		Context 			_context;

		Win::Event			_event;
		Win::CritSection	_critSect;
		int					_timeout;

		RequestQueue		_requests;
		auto_active<CommandIpc::ServerConversation> _activeConv;
		Win::AtomicCounter	_isIdle;
		Win::AtomicCounter	_activeConvKill;
		bool				_stayInGui;
		int					_lastProjectId;
	};
}
#endif
