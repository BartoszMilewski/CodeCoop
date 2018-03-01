// ---------------------------
// (c) Reliable Software, 2001
// ---------------------------

unit Package;

interface

procedure Register;

implementation

uses ToolsAPI, Integrator;

procedure Register;
begin
  RegisterPackageWizard (TIntegrator.Create as IOTAWizard);
end;

end.

