echo LocalEdit -- MergeSyncDel -- New
setlocal
set TestFile1=%User1TestRoot%\new.txt
set TestFile2=%User2TestRoot%\new.txt
checkin "%TestFile2%"
call TestState.bat "%TestFile2%" In
sleep 
call TestState.bat "%TestFile1%" In
removefile -d "%TestFile1%"
call TestState.bat "%TestFile1%" Del
checkin "%TestFile1%"
call TestState.bat "%TestFile1%" None
call LocalEdit.bat "%TestFile2%"
call SleepSync.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" MergeSyncDel
call Accept.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" New
endlocal
