echo In -- Del -- Merge Local Del -- Del -- Merge Local Del -- Sync -- In
setlocal
set TestFile1=%User1TestRoot%\in.txt
set TestFile2=%User2TestRoot%\in.txt
call TestState.bat "%TestFile2%" In
rem ---- Local Del + Merge + Local Del ---- 
call TestState.bat "%TestFile1%" In
call TestState.bat "%TestFile2%" In
removefile -d "%TestFile2%"
call TestState.bat "%TestFile2%" Del
call LocalEdit.bat "%TestFile1%"
call TestState.bat "%TestFile1%" Out
checkin "%TestFile1%"
call TestState.bat "%TestFile1%" In
call SleepUnpack.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" MergeLocalDel
call Accept.bat "%TestFile2%"
call TestState.bat "%TestFile2%" Del
rem ---- Local Del -- Merge -- Sync ----
call LocalEdit.bat "%TestFile1%"
call TestState.bat "%TestFile1%" Out
checkin "%TestFile1%"
call TestState.bat "%TestFile1%" In
call SleepUnpack.bat "%User2TestRoot%"
call TestState.bat "%TestFile2%" MergeLocalDel
uncheckout "%TestFile2%"
call TestState.bat "%TestFile2%" Sync
call Accept.bat "%TestFile2%"
call TestState.bat "%TestFile2%" In
endlocal

