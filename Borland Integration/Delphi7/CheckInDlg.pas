unit CheckInDlg;

interface

uses Windows, SysUtils, Classes, Graphics, Forms, Controls, StdCtrls,
  Buttons, ExtCtrls;

type TCheckInDlg = class(TForm)
    CommentMemo : TMemo;
    OKBtn: TButton;
    CancelBtn: TButton;
    procedure OKBtnClick (Sender: TObject);
    procedure CancelBtnClick (Sender: TObject);
public
    constructor Create (comment: Pointer);
private
    _comment : Pointer;
end;

implementation

{$R *.dfm}

constructor TCheckInDlg.Create (comment: Pointer);
begin
    inherited Create (nil);
    _comment := comment;
end;

procedure TCheckInDlg.OKBtnClick (Sender: TObject);
begin
    String (_comment^) := CommentMemo.Text;
    ModalResult := mrOK;
end;

procedure TCheckInDlg.CancelBtnClick (Sender: TObject);
begin
  ModalResult := mrCancel;
end;

end.
