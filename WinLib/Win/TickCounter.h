#if !defined (TICKCOUNTER_H)
#define TICKCOUNTER_H
//----------------------------------------------------
// TickCounter.h
// (c) Reliable Software 2000
//
//----------------------------------------------------

namespace Win
{
	class TickCounter
	{
	public:
		TickCounter (unsigned long interval)
			: _interval (interval)
		{
			_startTick = ::GetTickCount ();
		}
		bool IsDone () const
		{
			unsigned long curTick = ::GetTickCount ();
			int tickLeft;
			if (curTick <= _startTick)
			{
				// Wrap around
				tickLeft = _interval - ((0xffffffff - _startTick) + curTick);
			}
			else
			{
				tickLeft = _interval - (curTick - _startTick);
			}
			return tickLeft <= 0;
		}

	private:
		unsigned long _interval;
		unsigned long _startTick;
	};
}

#endif
