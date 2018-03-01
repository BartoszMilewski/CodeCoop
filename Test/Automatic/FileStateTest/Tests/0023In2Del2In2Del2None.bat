echo In -- Del -- In -- Del -- None
setlocal
set TestFile2=%User2TestRoot%\in.txt
call TestState.bat "%TestFile2%" In
removefile -d "%TestFile2%"
call TestState.bat "%TestFile2%" Del
uncheckout "%TestFile2%"
removefile -d "%TestFile2%"
call TestState.bat "%TestFile2%" Del
checkin "%TestFile2%"
call TestState.bat "%TestFile2%" None
endlocal

