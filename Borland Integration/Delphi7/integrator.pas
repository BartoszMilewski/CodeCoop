// -------------------------------
// (c) Reliable Software, 2001, 02
// -------------------------------

unit integrator;

interface

uses
  CoopMenu, SccProxy,
  Windows, ToolsAPI, Registry, SysUtils, Forms;

resourcestring CodeCoopExpert = 'Reliable Software.Code Co-op Expert';

type
  TIntegrator = class (TNotifierObject, IOTAWizard)
  public
    constructor Create;
    destructor Destroy; override;

    // IOTAWizard
    function GetIDString: string;
    function GetName: string;
    function GetState: TWizardState;
    procedure Execute ();

  protected
    _sccProxy : TSccProxy;
    _menu : TCoopMenuDataModule; // kind of Commander
end;

implementation

resourcestring SccDllName = '\sccdll.dll';

constructor TIntegrator.Create;
var
    coopRegistry : TRegistry;
    programPath : string;
begin
    inherited Create;

    programPath := '';
    coopRegistry := TRegistry.Create;
    try
        coopRegistry.RootKey := HKEY_LOCAL_MACHINE;
        if coopRegistry.OpenKey ('\Software\Reliable Software\Code Co-op', False) then
        begin
            programPath := coopRegistry.ReadString ('ProgramPath');
            coopRegistry.CloseKey;
        end
    finally
        coopRegistry.Free;
        inherited;
    end;
    if (programPath = '') then
    begin
        Application.MessageBox (
            'Cannot read Code Co-op registry. Run Co-op Setup again.',
            'Code Co-op...');
        Abort;
    end;

    _sccProxy := TSccProxy.Create (PChar (programPath + SccDllName));
    _menu := TCoopMenuDataModule.Create (_sccProxy);
end;

destructor TIntegrator.Destroy;
begin
  _menu.Free;
  _sccProxy.Free;

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

end.

