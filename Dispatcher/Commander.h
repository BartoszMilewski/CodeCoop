#if !defined (COMMANDER_H)
#define COMMANDER_H
//----------------------------------
// (c) Reliable Software 1998 - 2007
// ---------------------------------

#include "Global.h"
#include <Win/Message.h>
#include <Ctrl/Command.h>
#include <Win/Win.h>

class Model;
class ViewMan;
enum ViewType;
namespace Win { class MessagePrepro; }
class SelectionSeq;

class Commander
{
public:
	Commander (Win::Dow::Handle win, Win::MessagePrepro * msgPrepro, Model & model)
		: _win (win),
		  _msgPrepro (msgPrepro),
		  _model (model),
		  _msgCoopUpdate (UM_COOP_UPDATE),
		  _msgShowWindow (UM_SHOW_WINDOW)
	{}

    void Program_CoopUpdate ();                
    void Program_CoopUpdateOptions ();
	void Program_DistributorLicense ();
	Cmd::Status can_Program_DistributorLicense () const;
    void Program_ConfigWizard ();                
    void Program_Collaboration ();                
	void Program_Diagnostics ();
	Cmd::Status can_Program_EmailOptions () const;
	void Program_EmailOptions ();
	void Program_About ();                      
    void Program_Exit ();                       

	void View_GoUp ();
	Cmd::Status can_View_GoUp () const;
	void View_Quarantine ();
	Cmd::Status can_View_Quarantine () const;
	void View_AlertLog ();
	Cmd::Status can_View_AlertLog () const;
	void View_PublicInbox ();
	Cmd::Status can_View_PublicInbox () const;
	void View_Members ();
	Cmd::Status can_View_Members () const;
	void View_RemoteHubs ();
	Cmd::Status can_View_RemoteHubs () const;
	void View_Next ();
	void View_Previous ();

	Cmd::Status can_All_ReleaseFromQuarantine () const;
	void All_ReleaseFromQuarantine ();
	Cmd::Status can_All_PullFromHub () const;
	void All_PullFromHub ();
    void All_DispatchNow ();                        
    void All_SendMail ();                       
	Cmd::Status can_All_SendMail () const;
    void All_GetMail ();                       
	Cmd::Status can_All_GetMail () const;
	void All_Purge ();
	void All_ViewAlertLog ();                       
	Cmd::Status can_All_ViewAlertLog () const;
	void All_ClearAlertLog ();                       
	Cmd::Status can_All_ClearAlertLog () const;

	void Selection_ReleaseFromQuarantine ();
	Cmd::Status can_Selection_ReleaseFromQuarantine () const;
	void Selection_Details ();                  
	Cmd::Status can_Selection_Details () const;
    void Selection_Delete ();                 
	Cmd::Status can_Selection_Delete () const;

    void Help_Contents ();						

    void Window_Show ();						

	void SetView (ViewMan * viewMan) { _viewMan = viewMan; }

private:
	void Selection_HeaderDetails ();
	void Selection_ProjectMembers ();
	void Selection_EditTransport ();
	void Selection_InterClusterTransport ();

	bool IsIn (ViewType view) const;
	void RefreshUI ();	
	void ProposeUsingEmail ();

	void DeleteSelectedScripts (SelectionSeq & seq, bool isAllChunks = false);
private:
	Win::Dow::Handle		_win;
	Win::MessagePrepro *	_msgPrepro;
	Model &					_model;
    ViewMan  *				_viewMan;

	Win::RegisteredMessage	_msgCoopUpdate;
	Win::RegisteredMessage	_msgShowWindow;
};

#endif
