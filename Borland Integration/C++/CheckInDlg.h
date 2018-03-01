//----------------------------------------------------------------------------
#ifndef CheckInDlgH
#define CheckInDlgH
//----------------------------------------------------------------------------
#include <vcl\System.hpp>
#include <vcl\Windows.hpp>
#include <vcl\SysUtils.hpp>
#include <vcl\Classes.hpp>
#include <vcl\Graphics.hpp>
#include <vcl\StdCtrls.hpp>
#include <vcl\Forms.hpp>
#include <vcl\Controls.hpp>
#include <vcl\Buttons.hpp>
#include <vcl\ExtCtrls.hpp>

#include <string>
//----------------------------------------------------------------------------
class TCheckInDlg : public TForm
{
__published:
	TButton *OKBtn;
	TButton *CancelBtn;
	TBevel *Bevel1;
        TEdit *CommentEdit;
        TLabel *Label1;
        void __fastcall OKBtnClick(TObject *Sender);
        void __fastcall CancelBtnClick(TObject *Sender);
private:
  std::string & _comment;
public:
	virtual __fastcall TCheckInDlg (std::string & comment);
};
//----------------------------------------------------------------------------
extern PACKAGE TCheckInDlg *CheckInDlg;
//----------------------------------------------------------------------------
#endif    
