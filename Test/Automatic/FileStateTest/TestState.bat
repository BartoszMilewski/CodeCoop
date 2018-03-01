call filestate %1 %2
if errorlevel 1 goto error
goto end

:error
echo Test failed
echo Press CTRL-C to break
pause

:end
