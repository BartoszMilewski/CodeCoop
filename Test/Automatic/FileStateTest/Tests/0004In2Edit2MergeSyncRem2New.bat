echo LocalEdit -- MergeSyncRemove -- New
setlocal
set TestFile1=%User1TestRoot%\new.txt
set TestFile2=%User2TestRoot%\new.txt
call TestState.bat "%TestFile2%" In
call LocalEdit.bat "%TestFile2%"
call TestState.bat "%TestFile2%" Out
removefile "%TestFile1%"
call TestState.bat "%TestFile1%" Remove
checkin "%TestFile1%"
call TestState.bat "%TestFile1%" None
call SleepSync.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" MergeSyncRemove
call Accept.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" New
endlocal
