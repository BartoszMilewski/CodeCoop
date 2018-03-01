#if !defined (MESSENGERS_H)
#define MESSENGERS_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2007
//------------------------------------

#include <Win/EnumProcess.h>

class StateChangeNotify : public Win::Messenger
{
public:
	StateChangeNotify (int projectId, bool isNewMissing = false)
		: _projectId (projectId),
		  _isNewMissing (isNewMissing)
	{}

	void DeliverMessage (Win::Dow::Handle targetWindow);

private:
	int		_projectId;
	bool	_isNewMissing;
};

class BackupNotify : public Win::Messenger
{
public:
	BackupNotify (Win::Dow::Handle sourceWin)
		: _sourceWin (sourceWin),
		  _notification (false)
	{}

	bool NotificationSent () const { return _notification; }
	void DeliverMessage (Win::Dow::Handle targetWindow);

private:
	Win::Dow::Handle	_sourceWin;
	bool				_notification;
};

class CoopDetector : public Win::Messenger
{
public:
	CoopDetector (Win::Dow::Handle sourceWin)
		: _sourceWin (sourceWin),
		  _otherCoopInstance (false)
	{}

	bool MoreCoopInstancesDetected () const { return _otherCoopInstance; }

	void DeliverMessage (Win::Dow::Handle targetWindow);

private:
	Win::Dow::Handle	_sourceWin;
	bool				_otherCoopInstance;
};

#endif
