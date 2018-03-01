object CheckInDlg: TCheckInDlg
  Left = 245
  Top = 108
  BorderStyle = bsDialog
  Caption = 'Check-In Comment'
  ClientHeight = 206
  ClientWidth = 555
  Color = clBtnFace
  ParentFont = True
  OldCreateOrder = True
  Position = poScreenCenter
  PixelsPerInch = 120
  TextHeight = 16
  object Bevel1: TBevel
    Left = 10
    Top = 10
    Width = 535
    Height = 151
    Shape = bsFrame
  end
  object Label1: TLabel
    Left = 24
    Top = 112
    Width = 505
    Height = 41
    AutoSize = False
    Caption = 
      'We recommend starting Code Co-op for check-ins. Code Co-op offer' +
      's the possibility of checking-in mutliple files at once and to r' +
      'eview your changes before the check-in.'
    WordWrap = True
  end
  object OKBtn: TButton
    Left = 9
    Top = 166
    Width = 93
    Height = 30
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 0
    OnClick = OKBtnClick
  end
  object CancelBtn: TButton
    Left = 452
    Top = 166
    Width = 92
    Height = 30
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 1
    OnClick = CancelBtnClick
  end
  object CommentEdit: TEdit
    Left = 24
    Top = 24
    Width = 505
    Height = 81
    AutoSize = False
    TabOrder = 2
  end
end
