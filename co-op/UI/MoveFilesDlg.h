#if !defined (MOVEFILESDLG_H)
#define MOVEFILESDLG_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include <Win/Dialog.h>
#include <Ctrl/ListBox.h>
#include <Ctrl/Edit.h>
class NamedValues;
class WindowSeq;

class MoveFilesData
{
	friend class MoveFilesCtrl;
public:
	MoveFilesData(WindowSeq & selection, std::string const & currentDir, std::string const & rootDir);
	std::string const & GetTarget() const { return _targetDir; }
private:
	bool Verify(std::string const & path);
	void SetTarget(std::string const & target)
	{
		_targetDir = target;
	}
	WindowSeq & _names;
	std::string _targetDir;
	std::string _rootDir;
};

class MoveFilesCtrl : public Dialog::ControlHandler
{
public:
	MoveFilesCtrl (MoveFilesData & data);
    bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();
	bool GetDataFrom (NamedValues const & source);
private:
	MoveFilesData & _data;

	Win::ListBox::Simple	_fileListing;
	Win::Edit				_targetPath;
};

#endif
