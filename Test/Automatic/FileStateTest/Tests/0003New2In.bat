echo New -- In -- Sync (new) -- In
setlocal
set TestFile1=%User1TestRoot%\new.txt
set TestFile2=%User2TestRoot%\new.txt
call "%TestSuiteFolder%\0001None2New.bat"
checkin "%TestFile1%"
call TestState.bat "%TestFile1%" In
call SleepUnpack.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" Sync
call Accept.bat "%TestFile2%"
call TestState.bat "%TestFile2%" In
endlocal
