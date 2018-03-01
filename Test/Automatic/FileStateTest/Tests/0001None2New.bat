echo None -- New
setlocal
set TestFile=%User1TestRoot%\new.txt
copy "%Primitives%\new.txt" "%TestFile%"
addfile -t:text "%TestFile%"
call TestState.bat "%TestFile%" New
endlocal
