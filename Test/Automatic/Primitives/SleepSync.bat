echo Sleeping...
sleep %2
synch %1
if errorlevel 1 goto error
goto end

:error
echo Error synching project
pause

:end

