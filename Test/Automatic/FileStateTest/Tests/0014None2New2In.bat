echo None -- New (restored) -- In
setlocal
set TestFile1=%User1TestRoot%\in.txt
set TestFile2=%User2TestRoot%\in.txt

uncheckout "%TestFile2%"
call TestState.bat "%TestFile1%" In
call TestState.bat "%TestFile2%" In

restore -v:"previous" "%TestFile1%"
call TestState.bat "%TestFile1%" Remove
checkin "%TestFile1%"
call TestState.bat "%TestFile1%" None
call SleepSync.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" None
restore -v:"previous" "%TestFile2%"
call TestState.bat "%TestFile2%" New
checkin "%TestFile2%"
call TestState.bat "%TestFile2%" In
sleep
endlocal
