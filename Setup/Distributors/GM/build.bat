@echo off
checkout "d:\projects\co-op distributor\setup\help\help\support.html"
copy support.html ..\..\help\help
checkout "d:\projects\co-op distributor\co-op\resource\DistributorInfo.h"
copy DistributorInfo.h ..\..\..\co-op\Resource
cd ..\..
touch "d:\projects\co-op distributor\setup\help\help\support.html"
touch "d:\projects\co-op distributor\co-op\resource\DistributorInfo.h"
echo Ok, this is the time to compile the Co-op project. Press ENTER when this is done, to continue...
pause
call copyr.bat
echo Installation files are ready: Add them to zip and transform into self-extracting exe
pause
