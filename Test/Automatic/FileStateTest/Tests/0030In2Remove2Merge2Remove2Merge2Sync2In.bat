echo In -- Remove -- Merge Local Remove -- Remove -- Merge Local Remove -- Sync -- In
setlocal
set TestFile1=%User1TestRoot%\in.txt
set TestFile2=%User2TestRoot%\in.txt
call TestState.bat "%TestFile2%" None
rem ---- Add file to project ----
copy "%Primitives%\del.txt" "%TestFile2%"
addfile -t:text "%TestFile2%"
call TestState.bat "%TestFile2%" New
checkin "%TestFile2%"
sleep
rem ---- Local Del + Merge + Local Remove ---- 
call TestState.bat "%TestFile1%" In
call TestState.bat "%TestFile2%" In
removefile "%TestFile2%"
call TestState.bat "%TestFile2%" Remove
call LocalEdit.bat "%TestFile1%"
call TestState.bat "%TestFile1%" Out
checkin "%TestFile1%"
call TestState.bat "%TestFile1%" In
call SleepUnpack.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" MergeLocalRemove
call Accept.bat "%TestFile2%"
call TestState.bat "%TestFile2%" Remove
rem ---- Local Remove -- Merge -- Sync ----
call LocalEdit.bat "%TestFile1%"
call TestState.bat "%TestFile1%" Out
checkin "%TestFile1%"
call TestState.bat "%TestFile1%" In
call SleepUnpack.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" MergeLocalRemove
uncheckout "%TestFile2%"
call TestState.bat "%TestFile2%" Sync
call Accept.bat "%TestFile2%"
call TestState.bat "%TestFile2%" In
endlocal

