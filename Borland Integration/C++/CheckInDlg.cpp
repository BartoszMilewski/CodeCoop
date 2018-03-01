//---------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "CheckInDlg.h"
//---------------------------------------------------------------------
#pragma resource "*.dfm"
//---------------------------------------------------------------------
__fastcall TCheckInDlg::TCheckInDlg(std::string & comment)
	: TForm((TComponent*)0),
          _comment (comment)
{
}
//---------------------------------------------------------------------
void __fastcall TCheckInDlg::OKBtnClick(TObject *Sender)
{
    _comment = CommentEdit->Text.c_str ();
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------

void __fastcall TCheckInDlg::CancelBtnClick(TObject *Sender)
{
    ModalResult = mrCancel;        
}
//---------------------------------------------------------------------------

