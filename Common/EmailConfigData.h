#if !defined (EMAILCONFIGDLGDATA)
#define EMAILCONFIGDLGDATA
// ----------------------------------
// (c) Reliable Software, 2005 - 2007
// ----------------------------------

#include "EmailConfig.h"

class EmailConfigData
{
public:
	EmailConfigData (Email::RegConfig const & eCfg)
		: _receivePeriodInMin (eCfg.GetAutoReceivePeriodInMin ()),
		  _maxSize (eCfg.GetMaxEmailSize ())
	{
		_isAutoReceive = _receivePeriodInMin != 0;
	}
	unsigned long GetMaxEmailSize () const { return _maxSize; }
	bool IsAutoReceive () const { return _isAutoReceive; }
	unsigned int GetAutoReceiveInMin () const { return _receivePeriodInMin; }

	void SetMaxEmailSize (unsigned long newSize) { _maxSize = newSize; }
	void SetAutoReceiveInMin (unsigned int periodInMinutes) { _receivePeriodInMin = periodInMinutes; }
	void SetAutoReceive (bool flag) { _isAutoReceive = flag; }

	bool Validate () const;

private:
	bool			_isAutoReceive;
	unsigned int    _receivePeriodInMin;
	unsigned long	_maxSize;
};

#endif
