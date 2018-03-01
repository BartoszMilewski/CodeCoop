echo In -- Edit -- Merge -- Edit -- Del -- In
setlocal
set TestFile1=%User1TestRoot%\in.txt
set TestFile2=%User2TestRoot%\in.txt
call TestState.bat "%TestFile1%" In
call TestState.bat "%TestFile2%" In
call LocalEdit.bat "%TestFile2%"
call TestState.bat "%TestFile2%" Out
call LocalEdit.bat "%TestFile1%"
call TestState.bat "%TestFile1%" Out
checkin "%TestFile1%"
call TestState.bat "%TestFile1%" In
call SleepUnpack.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" Merge
call Accept.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" Out
removefile -d "%TestFile2%"
call TestState.bat "%TestFile2%" Del
uncheckout "%TestFile2%"
call TestState.bat "%TestFile2%" In
endlocal

