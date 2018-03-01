echo In -- Del -- Sync Remove -- None
setlocal
set TestFile1=%User1TestRoot%\del.txt
set TestFile2=%User2TestRoot%\del.txt
rem ---- Add file to project ----
copy "%Primitives%\del.txt" "%TestFile2%"
addfile -t:text "%TestFile2%"
call TestState.bat "%TestFile2%" New
checkin "%TestFile2%"
call TestState.bat "%TestFile2%" In
sleep
removefile -d "%TestFile2%"
call TestState.bat "%TestFile2%" Del
removefile "%TestFile1%"
call TestState.bat "%TestFile1%" Remove
checkin "%TestFile1%"
call SleepUnpack.bat %User2TestRoot%
call TestState.bat "%TestFile2%" SyncRemove
call Accept.bat "%TestFile2%"
call TestState.bat "%TestFile2%" None
endlocal

