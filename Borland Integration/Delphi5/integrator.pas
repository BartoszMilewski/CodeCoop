// ---------------------------
// (c) Reliable Software, 2001
// ---------------------------

unit integrator;

interface

uses
  Windows, ToolsAPI, Registry, Menus, Forms, SysUtils;

resourcestring CodeCoopExpert = 'Reliable Software.Code Co-op Expert';
resourcestring SccDll = '\sccdll.dll';
resourcestring AboutTxt = 'This is Code Co-op'#13#10'Version Control System'#13#10'by Reliable Software.'#13#10#10'Visit our web site at:'#13#10'www.ReliSoft.com';

type
  TIntegrator = class (TNotifierObject, IOTANotifier, IOTAWizard)
  public
    constructor Create;
    destructor Destroy; override;

    // IOTAWizard
    function GetIDString: string;
    function GetName: string;
    function GetState: TWizardState;
    procedure Execute ();
    // IOTANotifier
    procedure AfterSave;
    procedure BeforeSave;
    procedure Destroyed;
    procedure Modified;

  protected
    procedure OnClickCoopMenu (Sender : TObject);
    procedure OnClickLaunchCoop (Sender : TObject);
    procedure OnClickCheckout (Sender : TObject);
    procedure OnClickAbout (Sender : TObject);
  private
    _coopMenu : TMenuItem;
    _coopExe, _checkOut, _sep, _about : TMenuItem;

    _programPath : string;
    _currentFile : AnsiString;
    _notifierIndex : Integer;
end;

implementation

{ TIntegrator }

constructor TIntegrator.Create;
var
    mainMenu : TMainMenu;
    coopRegistry : TRegistry;
begin
    inherited Create;

    _programPath := '';
    coopRegistry := TRegistry.Create;
    try
        coopRegistry.RootKey := HKEY_LOCAL_MACHINE;
        if coopRegistry.OpenKey ('\Software\Reliable Software\Code Co-op', False) then
        begin
            _programPath := coopRegistry.ReadString ('ProgramPath');
            coopRegistry.CloseKey;
        end
    finally
        coopRegistry.Free;
        inherited;
    end;
    if (_programPath = '') then
    begin
        Application.MessageBox (
            'Cannot read Code Co-op registry. Run setup again.',
            'Code Co-op...', IDOK);
    end;

    // Create drop down menu
    _coopMenu := TMenuItem.Create (nil);
    _coopMenu.Caption := 'C&ode Coop';
    _coopMenu.Name    := 'CodeCoop';
    _coopMenu.OnClick := OnClickCoopMenu;

    // Insert drop down menu.
    mainMenu := (BorlandIDEServices as INTAServices).MainMenu;
    Assert(Assigned(mainMenu), 'MainMenu component not found');
    mainMenu.Items.Insert (mainMenu.Items.Count - 2, _coopMenu);

    _coopExe := TMenuItem.Create (_coopMenu);
    _coopExe.Caption := 'Launch Code Co-Op';
    _coopExe.Name := 'CodeCoop';
    _coopExe.OnClick := OnClickLaunchCoop;
    _coopExe.Enabled := _programPath <> '';
    _coopMenu.Add (_coopExe);

    _checkOut := TMenuItem.Create (_coopMenu);
    _checkOut.Caption := 'Check out';
    _checkOut.Name := 'Checkout';
    _checkOut.OnClick := OnClickCheckout;
    _checkOut.Enabled := _programPath <> '';
    _coopMenu.Add (_checkOut);

    _sep := TMenuItem.Create (_coopMenu);
    _sep.Caption := '-';
    _sep.Name := 'Separator';
    _coopMenu.Add (_sep);

    _about := TMenuItem.Create (_coopMenu);
    _about.Caption := 'About...';
    _about.Name := 'About';
    _about.OnClick := OnClickAbout;
    _about.Enabled := True;
    _coopMenu.Add (_about);

{
   Application.MessageBox ('Before add notifier', 'Add Notifier');
  _notifierIndex := (BorlandIDEServices as IOTAServices).AddNotifier (Self);
   Application.MessageBox ('After add notifier', 'Add Notifier');
  if _notifierIndex < 0 then
    Application.MessageBox ('Failed!', 'Add Notifier');
}

end;

destructor TIntegrator.Destroy;
begin
  _coopMenu.Free;
//  (BorlandIDEServices as IOTAServices).RemoveNotifier (_notifierIndex);

  inherited Destroy;
end;

// IOTAWizard
function TIntegrator.GetIDString: string;
begin
  Result := CodeCoopExpert;
end;

function TIntegrator.GetName: string;
begin
  Result := CodeCoopExpert;
end;

function TIntegrator.GetState: TWizardState;
begin
  Result := [wsEnabled];
end;

procedure TIntegrator.Execute ();
begin
    Assert (false); // never called !
end;

// IOTANotifier
procedure TIntegrator.AfterSave;
begin
    Application.MessageBox ('Hey, watch out!', 'After save', IDOK);
end;

procedure TIntegrator.BeforeSave;
begin
    Application.MessageBox ('Hey, watch out!', 'Before save', IDOK);
end;

procedure TIntegrator.Destroyed;
begin
    Application.MessageBox ('Hey, watch out!', 'Destroyed', IDOK);
end;

procedure TIntegrator.Modified;
begin
    Application.MessageBox ('Hey, watch out!', 'Modified', IDOK);
end;

// private
procedure TIntegrator.OnClickCoopMenu (Sender : TObject);
var
    moduleServices : IOTAModuleServices;
    currentModule : IOTAModule;

begin
    if (_programPath = '') then
        Exit;

    _currentFile := '';

    moduleServices := BorlandIDEServices as IOTAModuleServices;
    currentModule := moduleServices.CurrentModule ();

    if Assigned (currentModule) then
        _currentFile := currentModule.FileName;

    if (_currentFile <> '') then
    begin
        _checkOut.Enabled := True;
        _checkOut.Caption := 'Check-Out ' + _currentFile;
    end
    else
    begin
        _checkOut.Caption := 'Check-Out';
        _checkOut.Enabled := False;
    end;
end;

procedure TIntegrator.OnClickLaunchCoop (Sender : TObject);
type
    TSccRunScc = function (context : PAnsiChar;
                           hWnd : PInteger;
                           nFiles : Integer;
                           lpFileNames : PAnsiString) : Boolean; cdecl;

var
  h: Integer;
  sccRun : TSccRunScc;

begin
     Assert (_programPath <> '');

     h := LoadLibrary (PChar (_programPath + SccDll));
     if h <> 0 then
     begin
         @sccRun := GetProcAddress(h, 'SccRunScc');
         if @sccRun <> nil then
         begin
             sccRun (NIL,NIL,0,NIL);
         end;
         FreeLibrary(h);
     end;
end;

procedure TIntegrator.OnClickCheckout (Sender : TObject);
type
    TSccCheckout = function (context : PAnsiChar;
                                hWnd : Integer;
				nFiles : Integer;
				lpFileNames : PAnsiString;
                                lpComment : PAnsiString;
                                dwFlags : Integer;
                                pvOptions : PChar) : Boolean; cdecl;

var
    h : Integer;
    sccCheckout : TSccCheckout;
    dfmCurrentFile : AnsiString;
    attr : DWORD;
    fnameLen : Integer;
begin
     Assert (_programPath <> '');
     Assert (_currentFile <> '');

     h := LoadLibrary (PChar (_programPath + SccDll));
     if h <> 0 then
     begin
         @sccCheckout := GetProcAddress(h, 'SccCheckout');
         if @sccCheckout <> nil then
         begin
             sccCheckout (NIL, 0, 1, @_currentFile, NIL, 0, NIL);
             // for '*.pas' or '*.cpp' files also checkout corresponding '*.dfm' file
             fnameLen := Length (_currentFile) - 4;
             if ((AnsiPos ('.pas', _currentFile) - 1 = fnameLen) or
                 (AnsiPos ('.cpp', _currentFile) - 1 = fnameLen)) then
             begin
                 dfmCurrentFile := Copy (_currentFile, 0, Length (_currentFile) - 3);
                 dfmCurrentFile := Concat (dfmCurrentFile, 'dfm');
                 attr := GetFileAttributes (PChar (dfmCurrentFile));
                 if (attr <> $ffffffff) then
                     sccCheckout (NIL, 0, 1, @dfmCurrentFile, NIL, 0, NIL);
             end;
         end;
         FreeLibrary(h);
     end;

//   (BorlandIDEServices as IOTAActionServices).ReloadFile (currentFile);
//    or faster but hackish:

        attr := GetFileAttributes (PChar (_currentFile));
        if (attr <> $ffffffff) then
          if ((attr and FILE_ATTRIBUTE_READONLY) = 0) then
            (BorlandIDEServices as IOTAEditorServices).TopBuffer.SetIsReadOnly (False);
end;

procedure TIntegrator.OnClickAbout (Sender : TObject);
begin
    Application.MessageBox (PChar (AboutTxt), 'About...', IDOK);
end;

end.


