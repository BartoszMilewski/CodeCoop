#if !defined (HIDEEXTENSIONDLG_H)
#define HIDEEXTENSIONDLG_H
//------------------------------------
//  (c) Reliable Software, 2002 - 2007
//------------------------------------

#include <Ctrl/ListBox.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Ctrl/ProgressMeter.h>
#include <Ctrl/ProgressBar.h>
#include <Win/Dialog.h>

class HideExtensionCtrl : public Dialog::ControlHandler
{
public:
	HideExtensionCtrl (NocaseSet & extFilter,
					   bool & hideNotControlled,
					   std::string const & projectRoot,
					   bool scanProject = false);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	class ScanProgress : public Progress::Meter
	{
	public:
		ScanProgress (Win::ProgressBar & progressBar)
			: _progressBar (progressBar)
		{}

		void SetRange (int mini, int maxi, int step = 1)
		{
			_progressBar.SetRange (mini, maxi, step);
		}
		void StepIt ()
		{
			_progressBar.StepIt ();
		}

	private:
		Win::ProgressBar & _progressBar;
	};

private:
	void ScanProject ();
	void EnableDisableButtons ();

private:
	Win::ListBox::MultiSel	_alreadyHidden;
	Win::Button			_unHide;
	Win::Button			_hideSelectAll;
	Win::Button			_hideDeselectAll;
	Win::CheckBox		_hideNonProject;
    Win::Edit			_oneExtension;
	Win::Button			_add;
	Win::ListBox::MultiSel	_addList;
	Win::Button			_scanOrSelectAll;
	Win::Button			_addDeselectAll;
	Win::ProgressBar	_scanProgress;
	NocaseSet &			_extFilter;
	bool &				_hideNotControlled;
	std::string			_projectRoot;
	static const char  	_emptyProject [];
	bool				_projectScanned;
};

#endif
