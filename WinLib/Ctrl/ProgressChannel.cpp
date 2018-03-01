//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "WinLibBase.h"
#include "ProgressChannel.h"

#include <Ctrl/ProgressDialog.h>

Progress::Channel::Channel (bool canCancel)
	: _min (0),
	  _max (0),
	  _step (0),
	  _position (0)
{
	_change.set (Activity, true);
	_change.set (Cancelable, canCancel);
}

void Progress::Channel::SetRange (int min, int max, int step) throw ()
{
	Win::Lock lock (_critSection);
	if (_min != min || _max != max || _step != step)
	{
		_min = min;
		_position = min;
		_max = max;
		_step = step;
		_change.set (Range, true);
		_change.set (Canceled, false);
	}
}

bool Progress::Channel::GetRange (int & min, int & max, int & step)
{
	Win::Lock lock (_critSection);
	if (_change.test (Range))
	{
		min = _min;
		max = _max;
		step = _step;
		_change.set (Range, false);
		return true;
	}
	return false;
}

void Progress::Channel::PeekRange (int & min, int & max, int & step)
{
	Win::Lock lock (_critSection);
	min = _min;
	max = _max;
	step = _step;
}

void Progress::Channel::StepIt ()
{
	Win::Lock lock (_critSection);
	_position += _step;
	_change.set (Position, true);
}

void Progress::Channel::StepTo (int position)
{
	Win::Lock lock (_critSection);
	if (_position != position)
	{
		_position = position;
		_change.set (Position, true);
	}
}

bool Progress::Channel::GetPosition (int & position)
{
	Win::Lock lock (_critSection);
	if (_change.test (Position))
	{
		position = _position;
		_change.set (Position, false);
		return true;
	}
	return false;
}

void Progress::Channel::Cancel ()
{
	Win::Lock lock (_critSection);
	_change.set (Canceled,  true);
}

bool Progress::Channel::CanCancel ()
{
	Win::Lock lock (_critSection);
	return _change.test (Cancelable);
}

bool Progress::Channel::WasCanceled ()
{
	Win::Lock lock (_critSection);
	return _change.test (Cancelable) && _change.test (Canceled);
}

void Progress::Channel::SetActivity (std::string const & activity)
{
	Win::Lock lock (_critSection);
	_currentActivity = activity;
	_change.set (Activity, true);
}

bool Progress::Channel::GetActivity (std::string & activity)
{
	Win::Lock lock (_critSection);
	if (_change.test (Activity))
	{
		activity = _currentActivity;
		_change.set (Activity, false);
		return true;
	}
	return false;
}

void Progress::ChannelHandler::SetRange (int min, int max, int step)
{
	_channel.SetRange (min, max, step);
	_dialog.OnSetRange ();
}

void Progress::ChannelHandler::GetRange (int & min, int & max, int & step)
{
	_channel.PeekRange (min, max, step);
}

void Progress::ChannelHandler::SetActivity (std::string const & activity) 
{
	if (WasCanceled ())
		return;

	_channel.SetActivity (activity);
}

void Progress::ChannelHandler::StepIt () 
{
	if (WasCanceled ())
		return;

	_channel.StepIt ();
}

void Progress::ChannelHandler::StepTo (int step)
{
	if (WasCanceled ())
		return;

	_channel.StepTo (step);
}

void Progress::ChannelHandler::StepAndCheck () throw (Win::Exception)
{
	if (WasCanceled ())
		throw Win::Exception ();

	_channel.StepIt ();
}

void Progress::ChannelHandler::Close ()
{
	_dialog.Close ();
}

bool Progress::ChannelHandler::WasCanceled () 
{
	_dialog.PumpDialog ();
	return _channel.WasCanceled ();
}
