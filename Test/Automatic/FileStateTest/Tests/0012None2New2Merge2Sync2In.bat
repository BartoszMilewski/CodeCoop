echo None -- New (restored) -- None -- New (restored) -- Merge -- Uncheckout -- Sync -- In
setlocal
set TestFile1=%User1TestRoot%\in.txt
set TestFile2=%User2TestRoot%\in.txt
restore -v:"previous" "%TestFile2%"
call TestState.bat "%TestFile2%" New
uncheckout "%TestFile2%"
call TestState.bat "%TestFile2%" None
restore -v:"previous" "%TestFile2%"
call TestState.bat "%TestFile2%" New

restore -v:"previous" "%TestFile1%"
checkin "%TestFile1%"
call SleepUnpack.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" Merge
uncheckout "%TestFile2%"
call TestState.bat "%TestFile2%" Sync
call Accept.bat "%TestFile2%"
call TestState.bat "%TestFile2%" In
endlocal
