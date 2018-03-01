#if !defined (DYNAMICREBAR_H)
#define DYNAMICREBAR_H
//------------------------------------
//  (c) Reliable Software, 2006
//------------------------------------

#include <Win/Win.h>
#include <Ctrl/Rebar.h>

#include <auto_vector.h>

namespace Tool
{
	class DynamicRebar : public Notify::RebarHandler
	{
	public:
		class TipHandler : public Notify::ToolTipHandler
		{
			typedef std::vector<Tool::DynamicRebar const *> RebarList;

		public:
			TipHandler (unsigned id, Win::Dow::Handle win);

			void AddRebar (Tool::DynamicRebar const * rebar) { _rebars.push_back (rebar); }
			bool OnNeedText (Tool::TipForWindow * tip) throw ();
			bool OnNeedText (Tool::TipForCtrl * tip) throw ();

		private:
			RebarList	_rebars;
		};

	public:
		DynamicRebar (Win::Dow::Handle parentWin,
				   Cmd::Executor & executor,
				   Tool::Item const * buttonItems,
				   unsigned toolBoxId,
				   unsigned buttonCtrlId);
		~DynamicRebar ();

		Tool::ButtonBand * AddButtonBand (Tool::BandItem const & bandItem, Cmd::Vector & cmdVector);
		Tool::ButtonBand * GetButtonBand (unsigned bandId) { return _toolBandMap [bandId]; }
		Control::Handler * GetControlHandler (Win::Dow::Handle winFrom, 
											unsigned idFrom) throw ();
		Win::Dow::Handle GetParentWin () const { return _rebar.GetParent (); }
		Win::Dow::Handle GetWindow () const { return _rebar; }
		unsigned Height () const { return _height; }
		void Size (Win::Rect const & toolRect);
		void LayoutChange (unsigned int const * bandIds);
		void Disable ();
		void Enable ();
		void RefreshButtons ();
		virtual void RefreshTextField (std::string const & text) {}
		// Returns true when tool tip filled successfully
		bool FillToolTip (Tool::TipForCtrl * tip) const;
		bool FillToolTip (Tool::TipForWindow * tip) const;

		// Notify::RebarHandler
		bool OnChevronPushed (unsigned bandIdx,
							  unsigned bandId,
							  unsigned appParam,
							  Win::Rect const & chevronRect,
							  unsigned notificationParam) throw ();
		bool OnHeightChange () throw ();

		static unsigned const fudgeFactor = 3;	// In pixels

	private:
		typedef auto_vector<Tool::ButtonBand> BandContainer;
		typedef std::map<unsigned, Tool::ButtonBand *> Id2BandMap;	// Maps band id to Too::ButtonBand pointer
		typedef std::vector<Tool::ButtonBand *> ToolBandList;
		typedef std::vector<Tool::ButtonBand *>::const_iterator ToolBandSeq;

	private:
		Tool::Rebar			_rebar;
		BandContainer		_myToolBands;
		Id2BandMap			_toolBandMap;
		ToolBandList		_currentTools;

		Tool::Item const *	_buttonItems;
		Tool::ButtonCtrl	_buttonCtrl;
		unsigned int		_height;// cached rebar height
	};
}

#endif
