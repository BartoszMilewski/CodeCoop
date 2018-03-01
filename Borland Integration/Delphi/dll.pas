// ---------------------------
// (c) Reliable Software, 2001
// ---------------------------

unit Dll;

interface
uses ToolsAPI;

function InitExpert (const BorlandIDEServices : IBorlandIDEServices;
                     RegisterProc : TWizardRegisterProc;
                     var Terminate : TWizardTerminateProc) : Boolean; stdcall;

implementation

uses Integrator;

var ExpertIndex : Integer = -1;

procedure ExitExpert;
var
  WizardServices: IOTAWizardServices;
begin
  if ExpertIndex <> -1 then
  begin
    Assert (Assigned (BorlandIDEServices));

    WizardServices := BorlandIDEServices as IOTAWizardServices;
    Assert (Assigned (WizardServices));

    WizardServices.RemoveWizard (ExpertIndex);

    ExpertIndex := -1;
  end;
end;

function InitExpert (const BorlandIDEServices : IBorlandIDEServices;
                     RegisterProc : TWizardRegisterProc;
                     var Terminate : TWizardTerminateProc) : Boolean; stdcall;
var
  WizardServices : IOTAWizardServices;
begin
  Result := (BorlandIDEServices <> nil);

  if Result then
  begin
    Assert (ToolsApi.BorlandIDEServices = BorlandIDEServices);

    Terminate := ExitExpert;

    WizardServices := BorlandIDEServices as IOTAWizardServices;
    Assert (Assigned (WizardServices));

    ExpertIndex := WizardServices.AddWizard (TIntegrator.Create as IOTAWizard);

    Result := (ExpertIndex >= 0);
  end;
end;


exports
  InitExpert name WizardEntryPoint;

end.