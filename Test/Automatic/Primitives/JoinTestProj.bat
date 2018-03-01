@echo off
call SetTestsPath.bat
call SetTestsHubId.bat
call JoinProj.bat "%1" "%TestsPath%\%1%2" "%TestsHubId%" "%TestsHubId%"
