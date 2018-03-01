#if !defined (PROGRESSMETER_H)
#define PROGRESSMETER_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2007
//------------------------------------

namespace Progress
{
	class Meter
	{
	public:
		virtual ~Meter () {}
		virtual void SetRange (int min, int max, int step = 1) {}
		virtual void GetRange (int & min, int & max, int & step)
		{
			min = 0;
			max = 0;
			step = 1;
		}
		virtual void SetActivity (std::string const & activity) {}
		virtual void StepIt () {}
		virtual void StepTo (int step) {}
		virtual void StepAndCheck () throw (Win::Exception) {}
		virtual bool WasCanceled () { return false; }
		virtual void Close () {}
	};
}

#endif
