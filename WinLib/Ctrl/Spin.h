#if !defined (SPIN_H)
#define SPIN_H
// -----------------------------
// (c) Reliable Software 2000-03
// -----------------------------

#include "Controls.h"

namespace Win
{
	class Spin : public SimpleControl
	{
	public:
		Spin () {}
		Spin (Win::Dow::Handle winParent, int id)
			: SimpleControl (winParent, id)
		{}

		void SetRange (int min, int max)
		{
			CheckVal (min);
			CheckVal (max);
			SendMsg (UDM_SETRANGE, 0, MAKELPARAM (max, min));
		}
		void SetPos (int pos)
		{
			CheckVal (pos);
			SendMsg (UDM_SETPOS, 0, MAKELPARAM (pos, 0));
		}
	private:
		static void CheckVal (int val)
		{
			Assert (val >= UD_MINVAL && val <= UD_MAXVAL);
		}
	};
}

#endif
