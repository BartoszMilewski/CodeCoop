unit About;

interface

uses Windows, SysUtils, Classes, Graphics, Forms, Controls, StdCtrls, 
  Buttons, ExtCtrls;

type
  TAboutDlg = class (TForm)
    OKBtn: TButton;
    Bevel1: TBevel;
    Image1: TImage;
    StaticText1: TStaticText;
    StaticText2: TStaticText;
    StaticText3: TStaticText;
    StaticText4: TStaticText;
  private
    { Private declarations }
  public
    procedure OKBtnClick (Sender: TObject);
  end;

implementation

{$R *.dfm}

procedure TAboutDlg.OKBtnClick (Sender : TObject);
begin
  ModalResult := mrOK;
end;

end.
