#if !defined (SPLITTER_H)
#define SPLITTER_H
//----------------------------------
// Reliable Software (c) 1998 - 2005
//----------------------------------

#include <Win/Controller.h>
#include <Graph/Pen.h>
#include <Graph/Cursor.h>

#include <auto_vector.h>

namespace Splitter
{
	class Ctrl : public Win::Controller
	{
	public:
		Ctrl (unsigned splitterId)
			: _splitterId (splitterId),
			  _cx (0),
			  _cy (0),
			  _dragging (false),
			  _dragStart (0),
			  _dragX (0),
			  _dragY (0)
		{}

		bool OnCreate (Win::CreateData const * create, bool & success) throw ();
		bool OnSize (int width, int height, int flag) throw ()
		{
			_cx = width;
			_cy = height;
			return true;
		}
		bool OnLButtonDown (int x, int y, Win::KeyState kState) throw ();
		bool OnLButtonUp (int x, int y, Win::KeyState kState) throw ();
		bool OnMouseMove (int x, int y, Win::KeyState kState) throw ();
		bool OnCaptureChanged (Win::Dow::Handle newCaptureWin) throw ();

		virtual void BeginDrag (Win::Point const & parentOrigin,
								Win::Point const & splitterOrigin,
								int xMouse,
								int yMouse) = 0;
		virtual void UpdateDragPosition (int xMouse, int yMouse) = 0;
		virtual void DrawDivider (Win::UpdateCanvas & canvas) = 0;
		virtual void NotifyParent (int xMouse, int yMouse) = 0;

	protected:
		Win::Dow::Handle	_hParent;
		Win::Style			_parentStyle;
		unsigned			_splitterId;

		int					_cx;
		int					_cy;

		// just because user waves over splitter with mouse button down doesn't mean they're dragging it;
		// mouse down may have started in another control!!!
		bool				_dragging; 

		int					_dragStart;
		int					_dragX;
		int					_dragY;
		Pen::Pens3d			_pens;
	};

	class VerticalCtrl : public Ctrl
	{
	public:
		VerticalCtrl (unsigned splitterId)
			: Ctrl (splitterId)
		{}

		bool OnPaint () throw ();

		void BeginDrag (Win::Point const & parentOrigin,
						Win::Point const & splitterOrigin,
						int xMouse,
						int yMouse);
		void UpdateDragPosition (int xMouse, int yMouse);
		void DrawDivider (Win::UpdateCanvas & canvas);
		void NotifyParent (int xMouse, int yMouse);
	};

	class HorizontalCtrl : public Ctrl
	{
	public:
		HorizontalCtrl (unsigned splitterId)
			: Ctrl (splitterId)
		{}

		bool OnPaint () throw ();

		void BeginDrag (Win::Point const & parentOrigin,
						Win::Point const & splitterOrigin,
						int xMouse,
						int yMouse);
		void UpdateDragPosition (int xMouse, int yMouse);
		void DrawDivider (Win::UpdateCanvas & canvas);
		void NotifyParent (int xMouse, int yMouse);
	};

	class UseVertical
	{
	public:
		UseVertical (Win::Instance hInstance);
		~UseVertical ();
		Win::Dow::Handle MakeSplitter (Win::Dow::Handle hwndParent, unsigned splitterId);

	private:
		static char const *			_className;
		auto_vector<VerticalCtrl>	_ctrls;
	};

	class UseHorizontal
	{
	public:
		UseHorizontal (Win::Instance hInstance);
		~UseHorizontal ();
		Win::Dow::Handle MakeSplitter (Win::Dow::Handle hwndParent, unsigned splitterId);

	private:
		static char const *			_className;
		auto_vector<HorizontalCtrl>	_ctrls;
	};
};

#endif
