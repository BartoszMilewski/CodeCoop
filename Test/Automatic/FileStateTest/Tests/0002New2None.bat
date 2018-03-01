echo New -- None
setlocal
set TestFile=%User1TestRoot%\new.txt
uncheckout "%TestFile%"
call TestState.bat "%TestFile%" None
del "%TestFile%"
endlocal
