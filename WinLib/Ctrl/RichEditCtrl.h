#if !defined (RICHEDITCTRL_H)
#define RICHEDITCTRL_H
//------------------------------------------------
// RichEditCtrl.h
// (c) Reliable Software 2002
// -----------------------------------------------

#include "Edit.h"
#include <Graph/Font.h>
#include <Graph/Color.h>
#include <Win/Notification.h>

class Dll;

namespace Win
{
	class RichEdit : public Edit
	{
	public:
		RichEdit ();
		RichEdit (Win::Dow::Handle winParent, int id);
		~RichEdit ();

		Win::Edit::Style & Style () { return _style; }

		// parent receives child messages, canvas is where the child paints itself
		Win::Dow::Handle Create (Win::Dow::Handle winParent, Win::Dow::Handle canvas, int id);

		static int Millimeters2Twips (int millimeters);
		static int Points2Twips (int points);
		//	For selecting the end position
		static unsigned int EndPos ()
		{
			return std::numeric_limits<unsigned int>::max();
		}

		class CharFormat : public CHARFORMAT2
		{
		public:
			CharFormat ();
			CharFormat (CharFormat const & format);

			void SetFont (int pointSize, Font::Descriptor const & font);
			void SetFont (int pointSize, std::string const & faceName);
			void SetFontSize (int pointSize);
			void SetCharSet (unsigned char charSet);
			void SetPitchAndFamily (unsigned char pitchAndFamily);
			void SetFaceName (char const * faceName);
			void SetWeight (int weigth);
			void SetTextColor (Win::Color color);
			void SetBackColor (Win::Color color);
			void SetBold (bool bold = true);
			void SetItalic (bool italic = true);
			void SetStrikeOut (bool strikeOut = true);
		};

		class ParaFormat : public PARAFORMAT2
		{
		public:
			ParaFormat ();

			void SetLeftIdent (int twips, bool absolute = true);
			void SetSpaceBefore (int twips);
			void SetSpaceAfter (int twips);
			void SetTabStopEvery (int tabTwips);
		};

		using Edit::Append;

		int GetCurrentPosition () const;

		void Select (int start, int stop);
		void Append (char const * buf, CharFormat const & charFormat, ParaFormat const & paraFormat);
		void SetCharFormat (CharFormat const & charFormat, bool all = false);
		void SetParaFormat (ParaFormat const & paraFormat);
		void SetBackground (Win::Color color);
		void EnableResizeRequest ();
		void RequestResize ();

	private:
		void LoadCtrlDll ();
		char const * GetWinClassName () const;

	private:
		std::unique_ptr<Dll>	_editDll;	// Holds DLL implementing the rich edit control
		Win::Edit::Style	_style;
		bool				_isV2;
	};
}

namespace Notify
{
	// Subclass RichEditHandler overwriting some of its methods
	// In your controller, overwrite the following method to return your handler
	// Notify::Handler * Win::Controller::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom)
	class RichEditHandler : public Notify::Handler
	{
	public:
		explicit RichEditHandler (unsigned id) : Notify::Handler (id) {}
		virtual bool OnRequestResize (Win::Rect const & rect) throw ()
			{ return false; }

	protected:
		bool OnNotify (NMHDR * hdr, long & result);
	};
}

#endif
