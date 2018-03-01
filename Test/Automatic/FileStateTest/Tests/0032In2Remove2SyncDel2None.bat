echo In -- Remove -- Sync Del -- None
setlocal
set TestFile1=%User1TestRoot%\del.txt
set TestFile2=%User2TestRoot%\del.txt

addfile -t:text "%TestFile2%"
call TestState.bat "%TestFile2%" New
checkin "%TestFile2%"
call TestState.bat "%TestFile2%" In
sleep
removefile "%TestFile2%"
call TestState.bat "%TestFile2%" Remove
removefile -d "%TestFile1%"
call TestState.bat "%TestFile1%" Del
checkin "%TestFile1%"
call SleepUnpack.bat %User2TestRoot%
call TestState.bat "%TestFile2%" SyncDel
call Accept.bat "%TestFile2%"
call TestState.bat "%TestFile2%" None
endlocal

