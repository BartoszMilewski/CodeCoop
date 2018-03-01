//------------------------------------
//  (c) Reliable Software, 2002 - 2005
//------------------------------------

#include "precompiled.h"
#include "HideExtensionDlg.h"
#include "ExtensionScanner.h"
#include "Resource.h"

#include <File/Path.h>

char const HideExtensionCtrl::_emptyProject [] = "No files found";

HideExtensionCtrl::HideExtensionCtrl (NocaseSet & extFilter,
									  bool & hideNotControlled,
									  std::string const & projectRoot,
									  bool scanProject)
	: Dialog::ControlHandler (IDD_HIDE_EXTENSIONS),
	  _extFilter (extFilter),
	  _hideNotControlled (hideNotControlled),
	  _projectRoot (projectRoot),
	  _projectScanned (scanProject)
{}

bool HideExtensionCtrl::OnInitDialog () throw (Win::Exception)
{
	_alreadyHidden.Init (GetWindow (), IDC_HIDEN_EXTENSIONS);
	_unHide.Init (GetWindow (), IDC_UNHIDE);
	_hideSelectAll.Init (GetWindow (), IDC_HIDE_SELECT_ALL);
	_hideDeselectAll.Init (GetWindow (), IDC_HIDE_DESELECT_ALL);
	_hideNonProject.Init (GetWindow (), IDC_HIDE_NON_PROJECT);
    _oneExtension.Init (GetWindow (), IDC_ADD_EDIT);
	_add.Init (GetWindow (), IDC_ADD_EXTENSION);
	_addList.Init (GetWindow (), IDC_ADD_EXTENSIONS);
	_scanOrSelectAll.Init (GetWindow (), IDC_ADD_SCAN_OR_SELECT_ALL);
	_addDeselectAll.Init (GetWindow (), IDC_ADD_DESELECT_ALL);
	_scanProgress.Init (GetWindow (), IDC_SCAN_PROGRESS);

	for (NocaseSet::const_iterator iter = _extFilter.begin (); iter != _extFilter.end (); ++iter)
	{
		std::string const & ext = *iter;
		_alreadyHidden.AddItem (ext.c_str ());
	}

	if (_hideNotControlled)
		_hideNonProject.Check ();
	else
		_hideNonProject.UnCheck ();

	if (_projectScanned)
	{
		ScanProject ();
	}
	else
	{
		_addList.Show (SW_HIDE);
		_scanOrSelectAll.SetText ("Scan Project");
		_addDeselectAll.Show (SW_HIDE);
		_scanProgress.Show (SW_HIDE);
	}
	EnableDisableButtons ();
	return true;
}

bool HideExtensionCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_HIDE_SELECT_ALL:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_alreadyHidden.SelectAll ();
		}
		break;
	case IDC_HIDE_DESELECT_ALL:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_alreadyHidden.DeSelectAll ();
		}
		break;
	case IDC_UNHIDE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			std::vector<int> selection;
			_alreadyHidden.RetrieveSelection (selection);
			// By going backward, the indices are never invalidated
			for (std::vector<int>::reverse_iterator iter = selection.rbegin ();
				iter != selection.rend ();
				++iter)
			{
				int idx = *iter;
				std::string str;
				_alreadyHidden.GetItemString (idx, str);
				_addList.AddItem (str.c_str ());
				_alreadyHidden.DeleteItem (idx);
			}
			EnableDisableButtons ();
		}
		break;
	case IDC_ADD_EXTENSION:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_projectScanned)
			{
				std::vector<int> selection;
				_addList.RetrieveSelection (selection);
				// By going backward, the indices are never invalidated
				for (std::vector<int>::reverse_iterator iter = selection.rbegin ();
					iter != selection.rend ();
					++iter)
				{
					int idx = *iter;
					std::string str;
					_addList.GetItemString (idx, str);
					_alreadyHidden.AddItem (str.c_str ());
					_addList.DeleteItem (idx);
				}
			}
			else
			{
				std::string str (_oneExtension.GetString ());
				if (!str.empty () && str != _emptyProject)
				{
					_oneExtension.Clear ();
					PathMaker maker ("c:", "temp", "dummy", str.c_str ());
					PathSplitter splitter (maker);
					char const * ext = splitter.GetExtension ();
					if (_alreadyHidden.FindItemByName (ext) == Win::ListBox::NotFound)
						_alreadyHidden.AddItem (ext);
				}
			}
			EnableDisableButtons ();
		}
		break;
	case IDC_ADD_SCAN_OR_SELECT_ALL:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_projectScanned)
			{
				_addList.SelectAll ();
			}
			else
			{
				ScanProject ();
			}
		}
		break;
	case IDC_ADD_DESELECT_ALL:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_addList.DeSelectAll ();
		}
		break;
	case IDC_HIDEN_EXTENSIONS:
		if (_alreadyHidden.HasSelChanged (notifyCode))
		{
			if (_alreadyHidden.GetSelCount () == 0)
				_unHide.Disable ();
			else
				_unHide.Enable ();
		}
		break;
	}
	return true;
}

bool HideExtensionCtrl::OnApply () throw ()
{
	int itemCount = _alreadyHidden.GetCount ();
	_extFilter.clear ();
	for (int i = 0; i < itemCount; ++i)
	{
		std::string str;
		_alreadyHidden.GetItemString (i, str);
		_extFilter.insert (str);
	}
	_hideNotControlled = (itemCount == 0 || _hideNonProject.IsChecked ());
	EndOk ();
	return true;
}

void HideExtensionCtrl::ScanProject ()
{
	_scanProgress.Show ();
	ScanProgress progress (_scanProgress);
	ExtensionScanner scanner (_projectRoot, progress);
	if (!scanner.IsEmpty ())
	{
		NocaseSet const & extensions = scanner.GetExtensions ();
		for (NocaseSet::const_iterator iter = extensions.begin ();
			 iter != extensions.end ();
			 ++iter)
		{
			std::string const & ext = *iter;
			_addList.AddItem (ext.c_str ());
		}
		_oneExtension.Show (SW_HIDE);
		_scanOrSelectAll.SetText ("Select All");
		_addDeselectAll.Show ();
		_addList.Show ();
		_projectScanned = true;
	}
	else
	{
		_oneExtension.SetText (_emptyProject);
	}
	_scanProgress.Show (SW_HIDE);
	GetWindow ().Invalidate ();
}

void HideExtensionCtrl::EnableDisableButtons ()
{
	if (_alreadyHidden.GetCount () == 0)
	{
		_unHide.Disable ();
		_hideSelectAll.Disable ();
		_hideDeselectAll.Disable ();
		_hideNonProject.Check ();
		_hideNonProject.Disable ();
	}
	else
	{
		_hideSelectAll.Enable ();
		_hideDeselectAll.Enable ();
		_hideNonProject.Enable ();
	}
}
