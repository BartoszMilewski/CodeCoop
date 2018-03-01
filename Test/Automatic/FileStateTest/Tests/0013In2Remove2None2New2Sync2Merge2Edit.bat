echo In -- Remove -- None -- New (restored) -- Merge -- Out
setlocal
set TestFile1=%User1TestRoot%\in.txt
set TestFile2=%User2TestRoot%\in.txt
call TestState.bat "%TestFile2%" In
restore -v:"previous" "%TestFile2%"
call TestState.bat "%TestFile2%" Remove
checkin "%TestFile2%"
sleep
call TestState.bat "%TestFile1%" None
restore -v:"previous" "%TestFile1%"
call TestState.bat "%TestFile1%" New
restore -v:"previous" "%TestFile2%"
call TestState.bat "%TestFile2%" New
checkin "%TestFile1%"
call SleepUnpack.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" Merge
call Accept.bat "%TestFile2%"
call TestState.bat "%TestFile2%" Out
endlocal
