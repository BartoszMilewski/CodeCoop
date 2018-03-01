object CheckInDlg: TCheckInDlg
  Left = 271
  Top = 268
  BorderStyle = bsDialog
  Caption = 'Check-In Comment'
  ClientHeight = 166
  ClientWidth = 451
  Color = clBtnFace
  ParentFont = True
  OldCreateOrder = True
  Position = poScreenCenter
  PixelsPerInch = 96
  TextHeight = 13
  object Bevel1: TBevel
    Left = 8
    Top = 8
    Width = 433
    Height = 121
    Shape = bsFrame
  end
  object Label1: TLabel
    Left = 16
    Top = 79
    Width = 417
    Height = 42
    Alignment = taCenter
    AutoSize = False
    Caption = 
      'We recommend starting Code Co-op for check-ins. Code Co-op offer' +
      's the possibility of checking-in mutliple files at once and to r' +
      'eview your changes before the check-in.'
    Layout = tlCenter
    WordWrap = True
  end
  object OKBtn: TButton
    Left = 7
    Top = 136
    Width = 75
    Height = 25
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 1
    OnClick = OKBtnClick
  end
  object CancelBtn: TButton
    Left = 367
    Top = 136
    Width = 75
    Height = 25
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 2
    OnClick = CancelBtnClick
  end
  object CommentMemo: TMemo
    Left = 16
    Top = 16
    Width = 417
    Height = 65
    TabOrder = 0
  end
end
