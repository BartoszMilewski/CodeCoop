unit CoopMenu;

interface

uses
  SccProxy,
  Windows, ToolsAPI, SysUtils, Types, Classes, Menus, Controls, ImgList, ActnList;

type PIntegerDynArray = ^TIntegerDynArray;

type
  TCoopMenuDataModule = class(TDataModule)
    CoopMenu: TMainMenu;
    CoopActionList: TActionList;
    LaunchCoopAction: TAction;
    CoopMenuImageList: TImageList;
    CheckOutAction: TAction;
    CodeCoop: TMenuItem;
    LaunchCoop: TMenuItem;
    CheckOut: TMenuItem;
    UncheckOut: TMenuItem;
    CheckIn: TMenuItem;
    UncheckOutAction: TAction;
    Separator: TMenuItem;
    About: TMenuItem;
    CheckInAction: TAction;
    AboutAction: TAction;
    ShowDiffAction: TAction;
    ShowDifferences: TMenuItem;
    procedure LaunchCoopActionExecute(Sender: TObject);
    procedure CheckOutActionExecute(Sender: TObject);
    procedure CheckInActionExecute(Sender: TObject);
    procedure AboutActionExecute(Sender: TObject);
    procedure UncheckOutActionExecute(Sender: TObject);
    procedure CodeCoopClick(Sender: TObject);
    procedure ShowDiffActionExecute(Sender: TObject);
  private
    _sccProxy : TSccProxy;
  protected
    procedure InstallMenu;
    procedure GetCurrentModuleFiles (var files : TStringDynArray);
    procedure UpdateIDEReadOnlyFlag (files : PStringDynArray);

    function GetCurrentFile () : String;
    function IsControlled (filename : String) : Boolean;

    function IsAnyCheckedIn (statuses : PIntegerDynArray) : Boolean;
    function IsAnyNotCheckedIn (statuses : PIntegerDynArray) : Boolean;
    function IsAnyCheckedOut (statuses : PIntegerDynArray) : Boolean;
  public
    constructor Create (var sccProxy : TSccProxy);
    destructor Destroy; override;
end;

implementation

uses
  CheckInDlg,
  About,
  Forms;

{$R *.dfm}

constructor TCoopMenuDataModule.Create (var sccProxy : TSccProxy);
begin
  inherited Create (nil);
  _sccProxy := sccProxy;

  InstallMenu;
end;

destructor TCoopMenuDataModule.Destroy;
begin
  inherited Destroy;
end;

procedure TCoopMenuDataModule.InstallMenu;
var
  NTAServices: INTAServices;
  IdeMainMenu: TMainMenu;
  item: TMenuItem;
  i : Integer;
begin
  NTAServices := BorlandIDEServices as INTAServices;
  Assert (Assigned(NTAServices));

  IdeMainMenu := NTAServices.MainMenu;
  for i := 0 to CoopMenu.Items.Count - 1 do
  begin
    item := CoopMenu.Items [i];
    CoopMenu.Items.Remove (item);
    IdeMainMenu.Items.Insert (IdeMainMenu.Items.Count - 2, item);
  end;
end;

procedure TCoopMenuDataModule.CodeCoopClick (Sender: TObject);
var
  files : TStringDynArray;
  statuses : TIntegerDynArray;
begin
  GetCurrentModuleFiles (files);
  if Length (files) > 0 then
  begin
      _sccProxy.QueryStatuses (@files, statuses);

      CheckOutAction.Enabled   := IsAnyCheckedIn (@statuses);
      CheckInAction.Enabled    := IsAnyNotCheckedIn (@statuses);
      UncheckOutAction.Enabled := IsAnyCheckedOut (@statuses);
  end
  else
  begin
      CheckOutAction.Enabled   := False;
      CheckInAction.Enabled    := False;
      UncheckOutAction.Enabled := False;
  end;

  // Diff command is available when the current file is controlled
  ShowDiffAction.Enabled :=  IsControlled (GetCurrentFile ());
end;

function TCoopMenuDataModule.IsAnyCheckedIn (statuses : PIntegerDynArray) : Boolean;
var
  i : Integer;
begin
  Assert (Length (statuses^) > 0);
  Result := False;
  // scc flags:
  // 1 - controlled
  // 2 - checked-out
  for i := 0 to High (statuses^) do
  begin
    if (statuses^ [i] and 1) <> 0 then // controlled
      if (statuses^ [i] and 2) = 0 then // not checked-out
      begin
        Result := True;
        Break;
      end;
  end;
end;

function TCoopMenuDataModule.IsAnyNotCheckedIn (statuses : PIntegerDynArray) : Boolean;
var
  i : Integer;
begin
  Assert (Length (statuses^) > 0);
  Result := False;
  // scc flags:
  // 1 - controlled
  // 2 - checked-out
  for i := 0 to High (statuses^) do
  begin
    if (statuses^ [i] and 1) = 0 then // not controlled
    begin
      Result := True;
      Break;
    end
    else
      if (statuses^ [i] and 2) <> 0 then // checked-out
      begin
        Result := True;
        Break;
      end;
  end;
end;

function TCoopMenuDataModule.IsAnyCheckedOut (statuses : PIntegerDynArray) : Boolean;
var
  i : Integer;
begin
  Assert (Length (statuses^) > 0);
  Result := False;
  // scc flags:
  // 1 - controlled
  // 2 - checked-out
  for i := 0 to High (statuses^) do
  begin
    if (statuses^ [i] and 1) <> 0 then // controlled
      if (statuses^ [i] and 2) <> 0 then // checked-out
      begin
        Result := True;
        Break;
      end;
  end;
end;

function TCoopMenuDataModule.GetCurrentFile () : String;
var
    currentModule : IOTAModule;
    currentEditor : IOTAEditor;
begin
    currentModule := (BorlandIDEServices as IOTAModuleServices).CurrentModule ();
    if not Assigned (currentModule) then
      Exit;

    currentEditor := currentModule.CurrentEditor;
    if not Assigned (currentEditor) then
      Exit;

    Result := currentEditor.FileName;
end;

function TCoopMenuDataModule.IsControlled (filename : String) : Boolean;
var
  files    : TStringDynArray;
  statuses : TIntegerDynArray;
begin
    if Length (filename) = 0 then
    begin
        Result := False;
        Exit;
    end;

    SetLength (files, 1);
    files [0] := filename;
    _sccProxy.QueryStatuses (@files, statuses);
    Result := Odd (statuses [0]);
end;

procedure TCoopMenuDataModule.LaunchCoopActionExecute(Sender: TObject);
begin
  _sccProxy.LaunchCoop;
end;

procedure TCoopMenuDataModule.CheckOutActionExecute(Sender: TObject);
var
    files : TStringDynArray;
begin
    GetCurrentModuleFiles (files);
    _sccProxy.CheckOut (@files);
    UpdateIDEReadOnlyFlag (@files);
end;

procedure TCoopMenuDataModule.UncheckOutActionExecute (Sender: TObject);
var
    files : TStringDynArray;
begin
    GetCurrentModuleFiles (files);
    _sccProxy.UncheckOut (@files);
    UpdateIDEReadOnlyFlag (@files);
end;

procedure TCoopMenuDataModule.CheckInActionExecute (Sender: TObject);
var
    files : TStringDynArray;
    checkInDlg : TCheckInDlg;
    comment : String;

begin
  GetCurrentModuleFiles (files);
  checkInDlg := TCheckInDlg.Create (Addr (comment));
  if checkInDlg.ShowModal = mrOK then
  begin
    _sccProxy.AddNotControlled (@files);
    _sccProxy.CheckIn (@files, PChar (comment));
    UpdateIDEReadOnlyFlag (@files);
    checkInDlg.Free;
  end;
end;

procedure TCoopMenuDataModule.ShowDiffActionExecute(Sender: TObject);
var
  currFile : String;
begin
    currFile := GetCurrentFile ();
    if Length (currFile) > 0 then
      _sccProxy.ShowDifferences (PChar (currFile));
end;

procedure TCoopMenuDataModule.AboutActionExecute (Sender: TObject);
var
  aboutDlg : TAboutDlg;
begin
  aboutDlg := TAboutDlg.Create (nil);
  aboutDlg.ShowModal;
  aboutDlg.Free;
end;

procedure TCoopMenuDataModule.GetCurrentModuleFiles (var files : TStringDynArray);
var
    currentModule : IOTAModule;
    editor : IOTAEditor;
    i : Integer;
begin
    currentModule := (BorlandIDEServices as IOTAModuleServices).CurrentModule ();
    if not Assigned (currentModule) then
      Exit;

    SetLength (files, currentModule.ModuleFileCount);
    for i := 0 to currentModule.ModuleFileCount - 1 do
    begin
       editor := currentModule.ModuleFileEditors [i];
       files [i] := editor.FileName;
    end;
end;

procedure TCoopMenuDataModule.UpdateIDEReadOnlyFlag (files : PStringDynArray);
var
  attr : DWORD;
  isReadOnly : Boolean;
begin
    Assert (Length (files^) > 0);

    //   (BorlandIDEServices as IOTAActionServices).ReloadFile (currentFile);
    //    or faster but hackish:

   // Revisit: this method doesn't always work on *.dpk modules ?
   isReadOnly := False;
   attr := GetFileAttributes (PChar (files^ [0]));
   if (attr <> $ffffffff) then
   begin
     isReadOnly := (attr or FILE_ATTRIBUTE_READONLY) = attr;
     (BorlandIDEServices as IOTAEditorServices).TopBuffer.SetIsReadOnly (isReadOnly);
   end;
end;

end.
