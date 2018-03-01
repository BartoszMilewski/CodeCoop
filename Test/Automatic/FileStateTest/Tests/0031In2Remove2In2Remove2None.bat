echo In -- Remove -- In -- Remove -- None
setlocal
set TestFile2=%User2TestRoot%\in.txt
call TestState.bat "%TestFile2%" In
removefile "%TestFile2%"
call TestState.bat "%TestFile2%" Remove
uncheckout "%TestFile2%"
removefile "%TestFile2%"
call TestState.bat "%TestFile2%" Remove
checkin "%TestFile2%"
call TestState.bat "%TestFile2%" None
endlocal

