#if !defined (PROGRESSCHANNEL_H)
#define PROGRESSCHANNEL_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <Ctrl/ProgressMeter.h>

#include <Sys/Synchro.h>
#include <Bit.h>

namespace Progress
{
	class Dialog;

	// Progress::Channel contains shared data
	class Channel
	{
	public:
		Channel (bool canCancel);

		void SetRange (int min, int max, int step);
		bool GetRange (int & min, int & max, int & step);
		void PeekRange (int & min, int & max, int & step);
		void StepIt ();
		void StepTo (int position);
		bool GetPosition (int & position);

		void Cancel ();
		bool CanCancel ();
		bool WasCanceled ();

		void SetActivity (std::string const & activity);
		bool GetActivity (std::string & activity);

	private:
		enum Change
		{
			Activity,
			Range,
			Position,
			Cancelable,
			Canceled
		};

	private:
		Win::CritSection	_critSection;
		std::string			_currentActivity;
		int					_min;
		int					_max;
		int					_step;
		int					_position;
		BitSet<Change>		_change;
	};

	// Handles client interaction with progress meter via Progress::Channel and Progress::Dialog
	class ChannelHandler : public Progress::Meter
	{
	public:
		ChannelHandler (Progress::Channel & channel, Progress::Dialog & dialog)
			: _channel (channel),
			  _dialog (dialog)
		{}

		void SetRange (int min, int max, int step);
		void GetRange (int & min, int & max, int & step);
		void SetActivity (std::string const & activity);
		void StepIt ();
		void StepTo (int step);
		void StepAndCheck () throw (Win::Exception);
		bool WasCanceled ();
		void Close ();

	private:
		Progress::Channel &	_channel;
		Progress::Dialog &	_dialog;
	};
}

#endif
