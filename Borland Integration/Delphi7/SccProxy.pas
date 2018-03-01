// -------------------------------
// (c) Reliable Software, 2001, 02
// -------------------------------

unit SccProxy;

interface

uses Types;

type
    PStringDynArray = ^TStringDynArray;

    // SCC functions
    TSccRunScc = function (context : Pointer;
                           hWnd : Integer;
                           nFiles : Integer;
                           lpFileNames : PString) : Integer; cdecl;

    TSccCheckIn = function (context : Pointer;
                            hWnd : Integer;
                            nFiles : Integer;
                            lpFileNames : PString;
                            lpComment : PChar;
                            dwFlags : Integer;
                            pvOptions : PChar) : Integer; cdecl;

    TSccCheckout = function (context : Pointer;
                             hWnd : Integer;
                             nFiles : Integer;
                             lpFileNames : PString;
                             lpComment : PString;
                             dwFlags : Integer;
                             pvOptions : PChar) : Integer; cdecl;

    TSccUncheckOut = function (context : Pointer;
                               hWnd : Integer;
                               nFiles : Integer;
                               lpFileNames : PString;
                               lpComment : PString;
                               dwFlags : Integer;
                               pvOptions : PChar) : Integer; cdecl;

    TSccQueryInfo =  function (context : Pointer;
                               count : Integer;
                               lpFileNames : PString;
                               statuses : PInteger) : Integer; cdecl;
    TSccAdd = function (context : Pointer;
                        hwnd : Integer;
                        count : Integer;
                        lpFileNames : PString;
                        lpCommand : PChar;
                        flags : PInteger;
                        pvOptions : PChar) : Integer; cdecl;

    TSccDiff = function (context : Pointer;
                         hwnd : Integer;
                         lpFileName : PChar;
                         flags : Integer;
                         pvOptions : PChar) : Integer; cdecl;

  // ==============
  TSccProxy = class
  public
    constructor Create (sccPath : pchar);
    destructor Destroy; override;

    procedure LaunchCoop;
    procedure CheckIn (files : PStringDynArray; comment : PChar);
    procedure CheckOut (files : PStringDynArray);
    procedure UncheckOut (files : PStringDynArray);
    procedure AddNotControlled (files : PStringDynArray);
    procedure QueryStatuses (files : PStringDynArray; var statuses : TIntegerDynArray);
    procedure ShowDifferences (filename : PChar);

  private
    _h : Integer;
    _launch     : TSccRunScc;
    _checkIn    : TSccCheckIn;
    _checkOut   : TSccUncheckOut;
    _uncheckOut : TSccUncheckOut;
    _queryStatus: TSccQueryInfo;
    _add        : TSccAdd;
    _diff       : TSccDiff;

    procedure InitSccFunction (var sccProc : Pointer; sccProcName : pchar);
  end;

implementation

uses Windows, SysUtils, Forms;

constructor TSccProxy.Create (sccPath : pchar);
begin
  _h := LoadLibrary (sccPath);
  if _h = 0 then
  begin
    Application.MessageBox (
        'Cannot load SccDll.dll. Run Code Co-op Setup again.',
        'Code Co-op...');
    Abort;
  end;

  InitSccFunction (@_launch,     'SccRunScc');
  InitSccFunction (@_checkIn,    'SccCheckin');
  InitSccFunction (@_checkOut,   'SccCheckout');
  InitSccFunction (@_uncheckOut, 'SccUncheckout');
  InitSccFunction (@_queryStatus,'SccQueryInfo');
  InitSccFunction (@_add,        'SccAdd');
  InitSccFunction (@_diff,       'SccDiff');
end;

destructor TSccProxy.Destroy;
begin
  FreeLibrary (_h);
end;

procedure TSccProxy.LaunchCoop;
begin
  _launch (nil, 0, 0, nil);
end;

procedure TSccProxy.CheckIn (files : PStringDynArray; comment : PChar);
begin
  _checkIn (nil, 0, Length (files^), @(files^[0]), comment, 0, nil);
end;

procedure TSccProxy.CheckOut (files : PStringDynArray);
begin
  _checkOut (nil, 0, Length (files^), @(files^ [0]), nil, 0, nil);
end;

procedure TSccProxy.UncheckOut (files : PStringDynArray);
begin
  _uncheckOut (NIL, 0, Length (files^), @(files^[0]), NIL, 0, NIL);
end;

procedure TSccProxy.ShowDifferences (filename : PChar);
begin
  _diff (NIL, 0, filename, 0, NIL);
end;

procedure TSccProxy.AddNotControlled (files : PStringDynArray);
var
  notControlled : TStringDynArray;
  statuses      : TIntegerDynArray;
  i, size, idx  : Integer;
begin
  SetLength (statuses, Length (files^));
  _queryStatus (nil, Length (files^), @(files^[0]), @statuses [0]);

  size := 0;
  for i := 0 to High (statuses) do
    if not Odd (statuses [i]) then // SCC_STATUS_CONTROLLED is the youngest bit
      size := size + 1;            // so i check status oddity

  if size = 0 then
    Exit;

  // how to hell add items to dyn array without initializing its size first ??
  SetLength (notControlled, size);
  idx := 0;
  for i := 0 to High (notControlled) do
    if not Odd (statuses [i]) then
    begin
      notControlled [idx] := files^ [i];
      idx := idx + 1;
    end;
    _add (nil, 0, size, @notControlled [0], nil, nil, nil);

end;

procedure TSccProxy.QueryStatuses (files : PStringDynArray; var statuses : TIntegerDynArray);
begin
  SetLength (statuses, Length (files^));
  _queryStatus (nil, Length (files^), @(files^[0]), @statuses [0]);
end;

procedure TSccProxy.InitSccFunction (var sccProc : Pointer; sccProcName : pchar);
begin
  sccProc := nil;
  sccProc := GetProcAddress (_h, sccProcName);
  if sccProc = nil then
  begin
    Application.MessageBox (
        PChar ('Cannot find ' + sccProcName + ' in SccDll.dll. Run Code Co-op Setup again.'),
        'Code Co-op...');
    Abort;
  end;
end;

end.
