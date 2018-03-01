#if !defined (PROGRESSBAR_H)
#define PROGRESSBAR_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2007
//------------------------------------

#include <Ctrl/Controls.h>

namespace Win
{
	class ProgressBar : public SimpleControl
	{
	public:
		ProgressBar () {}
		ProgressBar (Win::Dow::Handle winParent, int id)
			: SimpleControl (winParent, id)
		{}
		ProgressBar (Win::Dow::Handle win)
			: SimpleControl (win)
		{}

		void ReSize (int left, int top, int width, int height)
		{
			Move (left, top, width, height);
		}

		void SetRange (int min, int max, int step);
		void StepIt ();
		void StepAndCheck () throw (Win::Exception) { StepIt (); }
		void StepTo (int min);
	};

	class ProgressBarMaker : public Win::ControlMaker
	{
	public:
		ProgressBarMaker (Win::Dow::Handle hParent, int id = 0);
	};
}

#endif
