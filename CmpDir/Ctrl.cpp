//------------------------------------
//  (c) Reliable Software, 2002 - 2005
//------------------------------------

#include "precompiled.h"
#include "Ctrl.h"
#include "CmpDirReg.h"
#include "OutSink.h"
#include "View.h"
#include "AccelTable.h"
#include "Commander.h"
#include "DataPortal.h"
#include "Resource/resource.h"
#include <Dbg/Out.h>
#include <Dbg/Log.h>
#include <Win/Message.h>
#include <Win/MsgLoop.h>
#include <CmdLineScanner.h>
#include <Win/Keyboard.h>

class ToolTipHandler : public Notify::ToolTipHandler
{
public:
	ToolTipHandler (Tool::Bar & toolBar)
		: Notify::ToolTipHandler (toolBar.GetId ()), _toolBar (toolBar)
	{}

	bool OnNeedText (Tool::TipForCtrl * tip) throw ()
	{
		_toolBar.FillToolTip (tip); 
		return true;
	}

private:
    Tool::Bar &	_toolBar;
};

class FileListHandler: public Notify::ListViewHandler
{
	class ItemKbdHandler: public Keyboard::Handler
	{
	public:
		ItemKbdHandler (Commander * commander)
			: _commander (commander)
		{}
		bool OnReturn () throw ();
	private:
		Commander * _commander;
	};
public:
	FileListHandler (View * view, Opener & opener, Commander * commander)
		: Notify::ListViewHandler (view->GetListView ().GetId ()), 
		  _view (view), 
		  _opener (opener),
		  _kbdHandler (commander)
	{}
	bool OnDblClick (); // throw ();
	bool OnItemChanged (Win::ListView::ItemState & state) throw ();
	Keyboard::Handler * GetKeyboardHandler () throw ()
	{
		return &_kbdHandler;
	}
private:
	View		  * _view;
	Opener		  & _opener;
	ItemKbdHandler	_kbdHandler;
};

bool FileListHandler::OnDblClick () // throw ()
{
	try
	{
		int i = _view->GetListView().GetFirstSelected ();
		if (i != -1)
		{
			_opener.Open (i);
			return true;
		}
	} catch (...) {}

	return false;
}

bool FileListHandler::ItemKbdHandler::OnReturn () throw ()
{
	if (_commander->can_Selection_View () == Cmd::Enabled)
		_commander->Selection_View ();
	return true;
}

bool FileListHandler::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
	_view->GetToolBarView ().Refresh ();
	return true;
}

class TabHandler : public Notify::TabHandler
{
public:
	TabHandler (Tab::Handle & tab, Cmd::Executor & executor)
		: Notify::TabHandler (tab.GetId ()),
		  _tab (tab),
		  _executor (executor)
	{}

	bool OnSelChange () throw ();
private:
	Tab::Handle &	_tab;
	Cmd::Executor &	_executor;
};

bool TabHandler::OnSelChange () throw ()
{
	switch (_tab.GetCurSelection ())
	{
	case View::TAB_NEW:
		_executor.ExecuteCommand ("View_New");
		return true;
		break;
	case View::TAB_OLD:
		_executor.ExecuteCommand ("View_Old");
		return true;
		break;
	case View::TAB_DIFF:
		_executor.ExecuteCommand ("View_Diff");
		return true;
		break;
	}
	return false;
}

// The main controller

CmpDirCtrl::CmpDirCtrl (char const * cmdParam, Win::MessagePrepro & msgPrepro)
	:_dataReadyMsg ("DataReadyMessage"),
	 _dataClearMsg ("DataClearMessage"),
	 _initMsg ("InitMessage"),
	 _msgPrepro (msgPrepro),
#pragma warning (disable:4355)
	 _sessionNew (*this, 0),
	 _sessionOld (*this, 1),
	 _sessionDiff (*this, 10),
	 _curSession (0)
#pragma warning (default:4355)
{
	CmdLineScanner cmdLine (cmdParam);
	if (cmdLine.Look () == CmdLineScanner::tokString)
	{
		_sessionOld.SetDir (cmdLine.GetString ());
		_sessionDiff.SetOldDir (cmdLine.GetString ());
		cmdLine.Accept ();
		if (cmdLine.Look () == CmdLineScanner::tokString)
		{
			_sessionNew.SetDir (cmdLine.GetString ());
			_sessionDiff.SetNewDir (cmdLine.GetString ());
			cmdLine.Accept ();
		}
	}
	//Dbg::TheLog.Open ("CmpDir.txt", "c:\\");
}

CmpDirCtrl::~CmpDirCtrl ()
{
}

bool CmpDirCtrl::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	Win::Dow::Handle win = GetWindow ();
	try
	{
		win.SetText ("Directory Differ");
		_commander.reset (new Commander (*this));
		// Create command vector
		_cmdVector.reset (new CmdVector (Cmd::Table, _commander.get ()));

		Accel::Maker accelMaker (Accel::Keys, *_cmdVector);
		_kbdAccel.reset (new Accel::Handler (_h, accelMaker.Create ()));
		_msgPrepro.SetKbdAccelerator (_kbdAccel.get ());
		_view.reset (new View (_h, *_cmdVector, *this));
		_commander->ConnectView (_view.get ());
		_viewHandler.reset (new FileListHandler (_view.get (), *this, _commander.get ()));
		_tabHandler.reset (new TabHandler (_view->GetTabView (), *this));
		_toolTipHandler.reset (new ToolTipHandler (_view->GetToolBarView ()));
		_menu.reset (new Menu::DropDown (Menu::barItems, *_cmdVector));
		_menu->AttachToWindow (_h);
		win.PostMsg (_initMsg);
		success = true;
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
		success = false;
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Initialization -- Unknown Error", Out::Error);
		success = false;
	}
	TheOutput.SetParent (win);
	return true;
}

bool CmpDirCtrl::OnCommand (int cmdId, bool isAccel) throw ()
{
	MenuCommand (cmdId);
	return true;
}

bool CmpDirCtrl::OnControl (Win::Dow::Handle control, unsigned id, unsigned notifyCode) throw ()
{
	if (_view->IsCmdButton (id))
	{
		ExecuteCommand (id);
		return true;
	}
	return false;
}

void CmpDirCtrl::MenuCommand (int cmdId) throw ()
{
	try
	{
		Cursor::Holder working (_hourglass);

		_cmdVector->Execute (cmdId);
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Internal Error: Command execution failure", Out::Error);
	}
}

bool CmpDirCtrl::IsEnabled (std::string const & cmdName) const throw ()
{
	return (_cmdVector->Test (cmdName.c_str ()) == Cmd::Enabled);
}

void CmpDirCtrl::ExecuteCommand (std::string const & cmdName) throw ()
{
	if (IsEnabled (cmdName))
	{
		int cmdId = _cmdVector->Cmd2Id (cmdName.c_str ());
		Assert (cmdId != -1);
		MenuCommand (cmdId);
	}
}

void CmpDirCtrl::ExecuteCommand (int cmdId) throw ()
{
	Assert (cmdId != -1);
	MenuCommand (cmdId);
}

bool CmpDirCtrl::OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ()
{
	if (state.IsDismissed ())
	{
		// Menu dismissed
		_view->DisplayStatus ("Ready");
	}
	else if (!state.IsSeparator () && !state.IsSysMenu () && !state.IsPopup ())
	{
		// Menu item selected
		char const * cmdHelp = _cmdVector->GetHelp (id);
		_view->DisplayStatus (cmdHelp);
	}
	else
	{
		return false;
	}
	return true;
}

bool CmpDirCtrl::OnInitPopup (Menu::Handle menu, int pos) throw ()
{
	try
	{
		_menu->RefreshPopup (menu, pos);
	}
	catch (...) 
	{
		Win::ClearError ();
		return false;
	}
	return true;
}

bool CmpDirCtrl::OnDestroy () throw ()
{
	try
	{
		Win::Placement placement (GetWindow ());
		Registry::CmpDirUser userWin ("Window");
		RegKey::Handle key = userWin.Key ();
		RegKey::SaveWinPlacement (placement,key );
		long width = _view->GetListView ().CalcColWidthPct ();
		key.SetValueLong ("ColWidth", width, true);

		Registry::CmpDirUser userPaths ("Paths");
		key = userPaths.Key ();
		key.SetValueString ("OldPath", _sessionOld.GetDir (), true);
		key.SetValueString ("NewPath", _sessionNew.GetDir (), true);
	}
	catch (...)
	{}
	Win::Quit ();
	return true;
}

bool CmpDirCtrl::OnSize (int width, int height, int flag) throw ()
{
	_view->Size (width, height); 
	return true;
}

Notify::Handler * CmpDirCtrl::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_viewHandler.get () && _viewHandler->IsHandlerFor (idFrom))
		return _viewHandler.get ();
	else if (_tabHandler.get () && _tabHandler->IsHandlerFor (idFrom))
		return _tabHandler.get ();
	else 
		return _toolTipHandler.get ();
}

Control::Handler * CmpDirCtrl::GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_view.get () != 0)
		return _view->GetControlHandler (winFrom, idFrom);
	return 0;
}

bool CmpDirCtrl::OnRegisteredMessage (Win::Message & msg) throw ()
{
	if (msg == _dataReadyMsg)
	{
		OnDataReady (msg.GetWParam () != 0);
		return true;
	}
	else if (msg == _dataClearMsg)
	{
		_view->Clear (_curSession->IsDiff ());
	}
	else if (msg == _initMsg)
	{
		OnInit ();
	}
	return false;
}

void CmpDirCtrl::OnInit ()
{
	unsigned long width = 75;
	// read from registry
	{
		Registry::CmpDirUser userWin ("Window");
		RegKey::Handle key = userWin.Key ();
		if (!key.GetValueLong ("ColWidth", width) || width <= 1 || width >= 100)
			width = 75;

		Registry::CmpDirUser userPaths ("Paths");
		key = userPaths.Key ();
		std::string oldPath = key.GetStringVal ("OldPath");
		std::string newPath = key.GetStringVal ("NewPath");
		if (!oldPath.empty () && _sessionOld.GetDir ().empty ())
		{
			_sessionDiff.SetOldDir (oldPath);
			_sessionOld.SetDir (oldPath);
		}
		if (!newPath.empty () && _sessionNew.GetDir ().empty ())
		{
			_sessionDiff.SetNewDir (newPath);
			_sessionNew.SetDir (newPath);
		}
	}
	_view->GetListView ().Initialize (width);
	StartSession (_sessionDiff);
}

// These are called by the commander

void CmpDirCtrl::StartSessionOld ()
{
	_view->SetTabOld ();
	StartSession (_sessionOld); 
}

void CmpDirCtrl::StartSessionNew ()
{
	_view->SetTabNew ();
	StartSession (_sessionNew);
}

void CmpDirCtrl::StartSessionDiff ()
{
	_view->SetTabDiff ();
	_sessionDiff.SetOldDir (_sessionOld.GetDir ());
	_sessionDiff.SetNewDir (_sessionNew.GetDir ());
	StartSession (_sessionDiff);
}

void CmpDirCtrl::StartSession (Session & session)
{
	if (_curSession)
		_curSession->Refresh (false);
	_curSession = &session;
	_view->Clear (_curSession->IsDiff ());
	_view->SetText (_curSession->GetCaption ());
	_curSession->Refresh (true);
}

void CmpDirCtrl::DirUp ()
{
	_curSession->DirUp ();
	_view->SetText (_curSession->GetCaption ());
}

void CmpDirCtrl::OnDataReady (bool isDone)
{

	{
		Win::Lock lock (_critSect);
		_data.EndBatch ();
		_view->InitMergeSources (_data.begin (), _data.end ());
	}

	_view->Merge ();

	if (!isDone)
		_view->DisplayStatus ("Working");
	else
	{
		_curSession->QueryDone ();
		_view->DisplayStatus ("Ready");
	}
}

void CmpDirCtrl::Open (int i)
{
	Data::Item const & item = _view->GetItem (i);
	if (item.IsFolder () || item.IsDrive ())
	{
		_curSession->DirDown (item.Name (), item.GetDiffState ());
		_view->SetText (_curSession->GetCaption ());
	}
	else
		_curSession->OpenFile (item.Name (), item.GetDiffState ());
}

void CmpDirCtrl::Refresh ()
{
	_curSession->Refresh (true);
} 

// called from another thread
void CmpDirCtrl::DataReady (Data::ChunkHolder data, bool isDone, int srcId)
{
	Win::Lock lock (_critSect);
	if (srcId == _curSession->GetId ())
	{
		// dbg << "Controller: Files from " << srcId << ", isDone = " << isDone << std::endl;
		// dbg << data << std::endl;
		_data.Push (data);
		_dataReadyMsg.SetWParam (isDone? 1: 0);
		GetWindow ().PostMsg (_dataReadyMsg);
	}
}

void CmpDirCtrl::DataClear ()
{
	Win::Lock lock (_critSect);
	_data.Clear ();
	GetWindow ().PostMsg (_dataClearMsg);
}
