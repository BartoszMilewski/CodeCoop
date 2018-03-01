//----------------------------------
// (c) Reliable Software 2000 - 2005
//----------------------------------

#include <WinLibBase.h>
#include <Ctrl/Controls.h>
#include <Win/Controller.h>
#include <Graph/Font.h>

namespace Win
{
	void SimpleControl::RegisterOwnerDraw (OwnerDraw::Handler * handler)
	{
		GetParent ().GetController ()->AddDrawHandler (handler);
	}
	
	void SimpleControl::UnregisterOwnerDraw ()
	{
		Win::Dow::Handle parent = GetParent ();
		if (!parent.IsNull ())
		{
			Win::Controller * ctrl = parent.GetController ();
			if (ctrl)
				ctrl->RemoveDrawHandler (parent, GetId ());
		}
	}

	void ControlMaker::RegisterOwnerDraw (OwnerDraw::Handler * handler)
	{
		handler->Attach (_hWndParent, reinterpret_cast<int> (_hMenu));
	}

	CommonControlsRegistry CommonControlsRegistry::_ccreg;

	void CommonControlsRegistry::Add(CommonControlsRegistry::Type types)
	{
		_icc.dwICC = types;
		if (!::InitCommonControlsEx(&_icc))
		{
			throw Win::Exception("InitCommonControlsEx failed");
		}
	}

	void ControlWithFont::SetFont (Font::Descriptor const & newFont)
	{
		Font::Maker maker (newFont);
		_font = maker.Create ();
		SetFont (_font);
	}

	void ControlWithFont::SetFont (int pointSize, std::string const & face)
	{
		Font::Maker maker (pointSize, face);
		_font = maker.Create ();
		SetFont (_font);
	}

	void UseCommonControls ()
	{
		::InitCommonControls ();
	}
}
