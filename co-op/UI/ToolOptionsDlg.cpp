//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "ToolOptionsDlg.h"
#include "ToolOptions.h"
#include "OutputSink.h"
#include "AppInfo.h"

#include <File/File.h>
#include <Com/Shell.h>
#include <Ctrl/FileGet.h>

//
// Editor options page
//

bool EditorOptionsDlg::OnInitDialog () throw (Win::Exception)
{
	_useBuiltInEditor.Init (GetWindow (), IDC_EDITOR_OPTIONS_BUILTIN);
	_useExternalEditor.Init (GetWindow (), IDC_EDITOR_OPTIONS_EXTERNAL);
	_path.Init (GetWindow (), IDC_EDITOR_PATH);
	_command.Init (GetWindow (), IDC_EDITOR_CMD);
	_browse.Init (GetWindow (), IDC_EDITOR_PATH_BROWSE);

	if (_options.UsesExternalEditor ())
	{
		_useExternalEditor.Check ();
		_path.SetString (_options.GetExternalEditorPath ());
		std::string cmdLine = _options.GetExternalEditorCommand ();
		if (cmdLine.empty ())
			cmdLine = "$file1";
		_command.SetString (cmdLine);
		_path.Enable ();
		_command.Enable ();
		_browse.Enable ();
	}
	else
	{
		_useBuiltInEditor.Check ();
		_path.Disable ();
		_command.Disable ();
		_browse.Disable ();
	}

	return true;
}

bool EditorOptionsDlg::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	switch (id)
	{
	case IDC_EDITOR_OPTIONS_BUILTIN:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_useBuiltInEditor.IsChecked ())
			{
				_path.Disable ();
				_command.Disable ();
				_browse.Disable ();
			}
		}
		break;

	case IDC_EDITOR_OPTIONS_EXTERNAL:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_useExternalEditor.IsChecked ())
			{
				_path.Enable ();
				_command.Enable ();
				_browse.Enable ();
			}
		}
		break;

	case IDC_EDITOR_PATH_BROWSE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			ShellMan::CommonProgramsFolder programs;
			FilePath path;
			programs.GetPath (path);

			FileGetter editorFile;
			editorFile.SetFilter ("Program File (*.exe)\0*.exe\0All Files (*.*)\0*.*");
			editorFile.SetInitDir (path.GetDir ());
			if (editorFile.GetExistingFile (GetWindow (), "Select External Editor Program"))
				_path.SetString (editorFile.GetPath ());
		}
		break;
	}
	return true;
}

bool EditorOptionsDlg::OnApply () throw ()
{
	if (_useBuiltInEditor.IsChecked ())
	{
		_options.SetUseExternalEditor (false);
	}
	else
	{
		std::string editorPath = _path.GetTrimmedString ();
		std::string editorCmd = _command.GetString ();
		if (File::Exists (editorPath.c_str ()))
		{
			_options.SetUseExternalEditor (true);
			_options.SetExternalEditorPath (editorPath);
			std::string::size_type pos = editorCmd.find ("$file1");
			if (pos == std::string::npos)
				editorCmd += " $file1";

			_options.SetExternalEditorCommand (editorCmd);
		}
		else
		{
			std::string info ("Cannot find editor program:\n\n");
			info += editorPath;
			TheOutput.Display (info.c_str (), Out::Information, GetWindow ());
			return false;
		}
	}
	return Dialog::ControlHandler::OnApply ();
}

//
// Differ options page
//

bool DifferOptionsDlg::OnInitDialog () throw (Win::Exception)
{
	_useBuiltInDiffer.Init (GetWindow (), IDC_DIFFER_OPTIONS_BUILTIN);
	_useBCDiffer.Init (GetWindow (), IDC_DIFFER_OPTIONS_BC);
	_useOriginalBCDiffer.Init (GetWindow (), IDC_DIFFER_OPTIONS_BC_ORIGINAL);
	_useCoopBCDiffer.Init (GetWindow (), IDC_DIFFER_OPTIONS_BUILTIN_BC);
	_useGuiffy.Init (GetWindow (), IDC_GUIFFY);
	_buyBC.Init (GetWindow (), IDC_BUTTON);
	_buyGuiffy.Init (GetWindow (), IDC_BUTTON2);

	_useBCDiffer.Disable ();
	_useOriginalBCDiffer.Disable ();
	_useCoopBCDiffer.Disable ();
	_useGuiffy.Disable ();

	// Enable options
	if (_options.HasBc ())
	{
		_useBCDiffer.Enable ();
		_useCoopBCDiffer.Enable ();
		if (_options.HasOriginalBc ())
			_useOriginalBCDiffer.Enable ();
	}
	if (_options.HasGuiffy ())
	{
		_useGuiffy.Enable ();
	}
	// Check options
	if (_options.UsesBc ())
	{
		_useBCDiffer.Check ();
		if (_options.UsesOriginalBc ())
			_useOriginalBCDiffer.Check ();
		else
			_useCoopBCDiffer.Check ();
	}
	else if (_options.UsesGuiffy ())
	{
		_useGuiffy.Check ();
	}
	else
	{
		_useBuiltInDiffer.Check ();
	}
	return true;
}

bool DifferOptionsDlg::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	switch (id)
	{
	case IDC_DIFFER_OPTIONS_BUILTIN:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_useBuiltInDiffer.IsChecked ())
			{
				if (_options.HasBc ())
				{
					_useOriginalBCDiffer.UnCheck ();
					_useCoopBCDiffer.UnCheck ();
				}
			}
		}
		break;

	case IDC_DIFFER_OPTIONS_BC:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_useBCDiffer.IsChecked ())
			{
				_useBuiltInDiffer.UnCheck ();
				_useCoopBCDiffer.Check ();
			}
		}
		break;

	case IDC_DIFFER_OPTIONS_BC_ORIGINAL:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_useOriginalBCDiffer.IsChecked ())
			{
				_useGuiffy.UnCheck ();
				_useBuiltInDiffer.UnCheck ();
				_useBCDiffer.Check ();
			}
		}
		break;

	case IDC_DIFFER_OPTIONS_BUILTIN_BC:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_useCoopBCDiffer.IsChecked ())
			{
				_useGuiffy.UnCheck ();
				_useBuiltInDiffer.UnCheck ();
				_useBCDiffer.Check ();
			}
		}
		break;
	case IDC_GUIFFY:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_useGuiffy.IsChecked ())
			{
				if (_options.HasBc ())
				{
					_useOriginalBCDiffer.UnCheck ();
					_useCoopBCDiffer.UnCheck ();
				}
			}
		}
		break;

	case IDC_BUTTON:
		{
			Win::Dow::Handle appWnd = TheAppInfo.GetWindow ();
			ResString wwwLink (appWnd, ID_WWW_PURCHASE_BC);
			ShellMan::Open (appWnd, wwwLink);
		}
		break;
	case IDC_BUTTON2:
		{
			Win::Dow::Handle appWnd = TheAppInfo.GetWindow ();
			ResString wwwLink (appWnd, ID_WWW_PURCHASE_GUIFFY);
			ShellMan::Open (appWnd, wwwLink);
		}
		break;
	}
	return true;
}

bool DifferOptionsDlg::OnApply () throw ()
{
	if (_useBuiltInDiffer.IsChecked ())
	{
		_options.MakeUseDefault (true);
	}
	else if (_useOriginalBCDiffer.IsChecked ())
	{
		if (File::Exists (_options.GetOriginalBcPath ().c_str ()))
		{
			_options.MakeUseOriginalBc (true);
		}
		else
		{
			std::string info ("Cannot find Beyond Compare differ program:\n\n");
			info += _options.GetOriginalBcPath ();
			info += "\n\nPlease, run Beyond Compare setup again.";
			TheOutput.Display (info.c_str (), Out::Information, GetWindow ());
			return false;
		}
	}
	else if (_useGuiffy.IsChecked ())
	{
		if (File::Exists (_options.GetGuiffyPath ().c_str ()))
		{
			_options.MakeUseGuiffy (true);
		}
		else
		{
			std::string info ("Cannot find Guiffy program:\n\n");
			info += _options.GetGuiffyPath ();
			info += "\n\nPlease, run Guiffy setup again.";
			TheOutput.Display (info.c_str (), Out::Information, GetWindow ());
			return false;
		}
	}
	else
	{
		if (File::Exists (_options.GetOurBcPath ().c_str ()))
		{
			_options.MakeUseOurBc (true);
		}
		else
		{
			std::string info ("Cannot find alternate differ program:\n\n");
			info += _options.GetOurBcPath ();
			info += "\n\nPlease, run Code Co-op setup again.";
			TheOutput.Display (info.c_str (), Out::Information, GetWindow ());
			return false;
		}
	}
	return Dialog::ControlHandler::OnApply ();
}

//
// Merger options page
//

bool MergerOptionsDlg::OnInitDialog () throw (Win::Exception)
{
	_useBCMerger.Init (GetWindow (), IDC_MERGER_OPTIONS_BC);
	_useOriginalBCMerger.Init (GetWindow (), IDC_MERGER_OPTIONS_BC_ORIGINAL);
	_useCoopBCMerger.Init (GetWindow (), IDC_MERGER_OPTIONS_BUILTIN_BC);
	_useGuiffy.Init (GetWindow (), IDC_GUIFFY);
	_buyBC.Init (GetWindow (), IDC_BUTTON);
	_buyGuiffy.Init (GetWindow (), IDC_BUTTON2);

	_useBCMerger.Enable ();
	_useOriginalBCMerger.Disable ();
	_useCoopBCMerger.Enable ();
	_useGuiffy.Disable ();

	// Enable options
	if (_options.HasBc ())
	{
		_useBCMerger.Enable ();
		_useCoopBCMerger.Enable ();
		if (_options.HasOriginalBc () && _options.BcSupportsMerge())
			_useOriginalBCMerger.Enable ();
	}
	if (_options.HasGuiffy ())
	{
		_useGuiffy.Enable ();
	}

	// Check options
	if (_options.UsesBc ())
	{
		_useBCMerger.Check ();
		if (_options.UsesOriginalBc ())
			_useOriginalBCMerger.Check ();
		else
			_useCoopBCMerger.Check ();
	}
	else if (_options.UsesGuiffy ())
	{
		_useGuiffy.Check ();
	}
	return true;
}

bool MergerOptionsDlg::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	switch (id)
	{
	case IDC_MERGER_OPTIONS_BC:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_useBCMerger.IsChecked ())
			{
				_useCoopBCMerger.Check ();
			}
		}
		break;

	case IDC_MERGER_OPTIONS_BC_ORIGINAL:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_useOriginalBCMerger.IsChecked ())
			{
				_useGuiffy.UnCheck ();
				_useBCMerger.Check ();
			}
		}
		break;

	case IDC_MERGER_OPTIONS_BUILTIN_BC:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_useCoopBCMerger.IsChecked ())
			{
				_useGuiffy.UnCheck ();
				_useBCMerger.Check ();
			}
		}
		break;
	case IDC_GUIFFY:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_useGuiffy.IsChecked ())
			{
				if (_options.HasBc ())
				{
					_useOriginalBCMerger.UnCheck ();
					_useCoopBCMerger.UnCheck ();
				}
			}
		}
		break;

	case IDC_BUTTON:
		{
			Win::Dow::Handle appWnd = TheAppInfo.GetWindow ();
			ResString wwwLink (appWnd, ID_WWW_PURCHASE_BC);
			ShellMan::Open (appWnd, wwwLink);
		}
		break;
	case IDC_BUTTON2:
		{
			Win::Dow::Handle appWnd = TheAppInfo.GetWindow ();
			ResString wwwLink (appWnd, ID_WWW_PURCHASE_GUIFFY);
			ShellMan::Open (appWnd, wwwLink);
		}
		break;
	}
	return true;
}

bool MergerOptionsDlg::OnApply () throw ()
{
	if (_useCoopBCMerger.IsChecked ())
	{
		if (File::Exists (_options.GetOurBcPath ().c_str ()))
		{
			_options.MakeUseOurBc (true);
		}
		else
		{
			std::string info ("Cannot find alternate merger program:\n\n");
			info += _options.GetOurBcPath ();
			info += "\n\nPlease, run Code Co-op setup again.";
			TheOutput.Display (info.c_str (), Out::Information, GetWindow ());
			return false;
		}
	}
	else if (_useOriginalBCMerger.IsChecked ())
	{
		if (File::Exists (_options.GetOriginalBcPath ().c_str ()))
		{
			_options.MakeUseOriginalBc (true);
		}
		else
		{
			std::string info ("Cannot find Beyond Compare merger program:\n\n");
			info += _options.GetOriginalBcPath ();
			info += "\n\nPlease, run Beyond Compare setup again.";
			TheOutput.Display (info.c_str (), Out::Information, GetWindow ());
			return false;
		}
	}
	else if (_useGuiffy.IsChecked ())
	{
		if (File::Exists (_options.GetGuiffyPath ().c_str ()))
		{
			_options.MakeUseGuiffy (true);
		}
		else
		{
			std::string info ("Cannot find Guiffy program:\n\n");
			info += _options.GetGuiffyPath ();
			info += "\n\nPlease, run Guiffy setup again.";
			TheOutput.Display (info.c_str (), Out::Information, GetWindow ());
			return false;
		}
	}
	else
	{
		_options.MakeUseDefault (true);
	}
	return Dialog::ControlHandler::OnApply ();
}
