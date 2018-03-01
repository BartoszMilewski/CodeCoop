#if !defined (FOCUSBARWIN_H)
#define FOCUSBARWIN_H
//----------------------------------
// Reliable Software (c) 2005 - 2006
//----------------------------------

#include <Win/Controller.h>
#include <Win/ControlHandler.h>
#include <Graph/Pen.h>

#include <auto_vector.h>

namespace Focus
{
	class Ring;
}

namespace FocusBar
{
	// A bar with text, usually positioned above one of child windows 
	// members of a Focus::Ring
	// When the user clicks on the bar, the focus is given to the child
	// window that is associated with the bar.
	// On the other hand, when the associated child gets focus
	// it should call TurnOn, and when it loses focus, it should call Clear
	class Ctrl : public Win::Controller, public Control::Handler
	{
	public:
		Ctrl (unsigned id, unsigned associatedId, Focus::Ring & focusRing)
			: Control::Handler (id),
			  _id (id),
			  _associatedId (associatedId),
			  _focusRing (focusRing),
			  _isOn (false),
			  _cx (0),
			  _cy (0),
			  _fontHeight (0)
		{}
		void SetTitle (std::string const & title) { _title = title; }
		unsigned GetId () const { return _id; }
		int GetHeight () const;
		void TurnOn ();
		void TurnOff ();

		bool OnCreate (Win::CreateData const * create, bool & success) throw ();
		bool OnSize (int width, int height, int flag) throw ();
		bool OnMouseActivate (Win::HitTest hitTest, Win::MouseActiveAction & activate) throw ();
		bool OnPaint () throw ();
		bool OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ();
		Control::Handler * GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
		{
			return this;
		}
		// ControlHandler
		bool OnControl (unsigned id, unsigned notifyCode) throw ();

	private:
		static const unsigned CLOSE_BUTTON_ID = 15;

		void UseSystemSettings ();

	protected:
		unsigned			_id;
		unsigned			_associatedId;
		Focus::Ring &		_focusRing;
		std::string			_title;
		bool				_isOn;

		int					_cx;
		int					_cy;
		Pen::Pens3d			_pens;
		Font::AutoHandle	_font;
		int					_fontHeight;
		Win::Dow::Handle	_closeButton;
	};

	class Use
	{
	public:
		Use (Win::Instance hInstance);
		~Use ();
		FocusBar::Ctrl * MakeBarWindow (Win::Dow::Handle hwndParent,
										unsigned id,
										unsigned associatedId,
										Focus::Ring & focusRing);

	private:
		static char const *			_className;
		auto_vector<FocusBar::Ctrl>	_ctrls;
	};
};

#endif
