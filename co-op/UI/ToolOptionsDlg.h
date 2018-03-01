#if !defined (DIFFERTOOLDLG_H)
#define DIFFERTOOLDLG_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "Resource.h"

#include <Win/Dialog.h>
#include <Ctrl/Static.h>
#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>

namespace ToolOptions
{
	class Editor;
	class Differ;
	class Merger;
}

class EditorOptionsDlg : public Dialog::ControlHandler
{
public:
	EditorOptionsDlg (ToolOptions::Editor & options)
		: Dialog::ControlHandler (IDD_EDITOR_OPTIONS),
		  _options (options)
	{}

	bool OnApply () throw ();

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	Win::RadioButton		_useBuiltInEditor;
	Win::RadioButton		_useExternalEditor;
	Win::Edit				_path;
	Win::Edit				_command;
	Win::Button				_browse;
	ToolOptions::Editor &	_options;
};

class DifferOptionsDlg : public Dialog::ControlHandler
{
public:
	DifferOptionsDlg (ToolOptions::Differ & options)
		: Dialog::ControlHandler (IDD_DIFFER_OPTIONS),
		  _options (options)
	{}

	bool OnApply () throw ();

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	Win::RadioButton		_useBuiltInDiffer;
	Win::RadioButton		_useBCDiffer;
	Win::RadioButton		_useOriginalBCDiffer;
	Win::RadioButton		_useCoopBCDiffer;
	Win::RadioButton		_useGuiffy;
	Win::Button				_buyBC;
	Win::Button				_buyGuiffy;
	ToolOptions::Differ &	_options;
};

class MergerOptionsDlg : public Dialog::ControlHandler
{
public:
	MergerOptionsDlg (ToolOptions::Merger & options)
		: Dialog::ControlHandler (IDD_MERGER_OPTIONS),
		  _options (options)
	{}

	bool OnApply () throw ();

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	Win::RadioButton		_useBCMerger;
	Win::RadioButton		_useOriginalBCMerger;
	Win::RadioButton		_useCoopBCMerger;
	Win::RadioButton		_useGuiffy;
	//Win::RadioButton		_useAraxis;
	Win::Button				_buyBC;
	Win::Button				_buyGuiffy;
	ToolOptions::Merger &	_options;
};

#endif
