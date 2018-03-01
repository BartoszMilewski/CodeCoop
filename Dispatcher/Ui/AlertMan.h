#if !defined (ALERTMAN_H)
#define ALERTMAN_H
// ---------------------------------
// (c) Reliable Software 2002 - 2006
// ---------------------------------
#include <Sys/Synchro.h>
#include <Win/Message.h>
#include <queue>

namespace Win { class Exception; }
class AlertLog;

class AlertMan
{
public:
	enum AlertType
	{
		Unknown,
		Quarantine,
		Update,
		Info
	};
public:
	AlertMan ();
	void SetWindow (Win::Dow::Handle win) {	_win = win;	}
	void SetAlertLog (AlertLog * log) { _alertLog = log; }

	void PostQuarantineAlert (std::string const & msg);
	void PostUpdateAlert (std::string const & msg);
	void PostInfoAlert (std::string const & msg, 
						bool isVerbose = true, 
						bool isLowPriority = false,
						std::string const & hint = std::string ());
	void PostInfoAlert (Win::Exception const & e, bool isVerbose = true) throw ();

	void Display ();

	bool IsAlerting () const { return !_visibleAlerts.empty (); }
	AlertType PopAlert (bool & isLowPriority);

private:
	class AlertMsg
	{
	public:
		AlertMsg ()
			: _type (Unknown),
			  _isVerbose (false),
			  _isLowPriority (false)
		{}
		AlertMsg (AlertType type, 
				  std::string const & msg, 
				  bool isVerbose, 
				  bool isLowPriority)
			: _type (type),
			  _msg (msg), 
			  _isVerbose (isVerbose),
			  _isLowPriority (isLowPriority)
		{
			Assert (_type != Unknown);
		}
		AlertMan::AlertType	_type;
		std::string			_msg;
		bool				_isVerbose;
		bool				_isLowPriority;
	};

	class SharedAlertMsgQueue
	{
	public:
		void PushBack (AlertMsg const & alert)
		{
			Win::Lock lock (_critSection);
			_alerts.push (alert);
		}
		bool PopFront (AlertMsg & msg)
		{
			Win::Lock lock (_critSection);
			if (_alerts.empty ())
				return false;

			msg = _alerts.front ();
			_alerts.pop ();
			return true;
		}
	private:
		Win::CritSection	    _critSection;
		std::queue<AlertMsg>	_alerts;
	};
private:
	void AddNotify (AlertMsg const & alert);

private:
	Win::Dow::Handle		_win;
	Win::RegisteredMessage	_msgDisplayAlert;
	
	SharedAlertMsgQueue		_requestedAlerts;

	typedef std::pair<AlertType, bool> TypePriorityPair;
	std::queue<TypePriorityPair>	_visibleAlerts;

	AlertLog			  * _alertLog;
};

extern AlertMan TheAlertMan;

#endif
