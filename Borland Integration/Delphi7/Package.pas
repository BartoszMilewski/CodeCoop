// -------------------------------
// (c) Reliable Software, 2001, 02
// -------------------------------

unit Package;

interface

procedure Register;

implementation

uses ToolsAPI, Integrator;

procedure Register;
var
  wizard : TIntegrator;
begin
  try
    wizard := TIntegrator.Create;
  finally
  end;
  RegisterPackageWizard (wizard);
end;

end.

