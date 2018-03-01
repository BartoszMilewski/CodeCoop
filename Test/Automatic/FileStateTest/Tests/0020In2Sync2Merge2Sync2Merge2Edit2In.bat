echo In -- Sync -- Merge -- Sync -- Merge -- Edit -- In
setlocal
set TestFile1=%User1TestRoot%\in.txt
set TestFile2=%User2TestRoot%\in.txt
call TestState.bat "%TestFile1%" In
call TestState.bat "%TestFile2%" In
call LocalEdit.bat "%TestFile1%"
call TestState.bat "%TestFile1%" Out
checkin "%TestFile1%"
call TestState.bat "%TestFile1%" In
call SleepUnpack.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" Sync
checkout "%TestFile2%"
call TestState.bat "%TestFile2%" Merge
uncheckout "%TestFile2%"
call TestState.bat "%TestFile2%" Sync
checkout "%TestFile2%"
call TestState.bat "%TestFile2%" Merge
call Accept.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" Out
checkin "%TestFile2%"
call TestState.bat "%TestFile2%" In
endlocal

