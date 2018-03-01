@echo off
call SetTestsPath.bat
call SetTestsHubId.bat
call NewProj.bat "%1" "%TestsPath%\%1" "%TestsHubId%"
