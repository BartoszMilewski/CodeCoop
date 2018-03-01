#if !defined (ICON_H)
#define ICON_H
// -----------------------------------
// (c) Reliable Software, 2000 -- 2004
// -----------------------------------

namespace Win { class Instance; }

namespace Icon
{
	class SharedMaker
	{
	public:
	    SharedMaker (int width = 0, int height = 0)
			: _options (LR_SHARED),
			  _width (width),
			  _height (height)
		{}
		void SetSize (int width, int height)
		{
			_width = width;
			_height = height;
		}
		// set other options

		Icon::Handle Load (Win::Instance inst, unsigned id);
		Icon::Handle Load (char const * iconName);

	protected:
		unsigned _options;
		int		_width;
		int		_height;
	};

	class SmallMaker: public SharedMaker
	{
	public:
		SmallMaker ();
	};

	class StdMaker: public SharedMaker
	{
	public:
		StdMaker ();
	};
}

#endif
