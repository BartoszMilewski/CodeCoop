@echo off

setlocal
set User1TestRoot=d:\projects\tests\FileStateTestUser1
set User2TestRoot=d:\projects\tests\FileStateTestUser2
set SourceFolder=d:\projects\rebecca\test\automatic
set Primitives=%SourceFolder%\primitives
set Path=%Path%;%Primitives%
set TestSuiteFolder=%SourceFolder%\FileStateTest\Tests
set TesterHubId=piotr@trojanowski.a4.pl

echo Creating project in %User1TestRoot%
mkdir %User1TestRoot%
call NewProj "FileStateTest" "%User1TestRoot%" "%TesterHubId%"
coopcmd "%User1TestRoot%" -c:"Project_Options autosynch:on autojoin:on"

echo Creating project in %User2TestRoot%
mkdir %User2TestRoot%
call JoinProj "FileStateTest" "%User2TestRoot%" "%TesterHubId%" "%TesterHubId%"
echo Waiting for a Full Sync
call SleepSync.bat "%User2TestRoot%" 10

echo Testing...
for %%f in (%TestSuiteFolder%\*.bat) do call "%Primitives%\RunSingleTest.bat" %%f
echo.
echo Testing passed successfully! ;-) Well done!
echo.

call DefectProj "%User2TestRoot%"
call DefectProj "%User1TestRoot%"

set User1TestRoot=
set User2TestRoot=
set Primitives=
set SourceFolder=
set TestSuiteFolder=
set TesterHubId=
endlocal

echo on
