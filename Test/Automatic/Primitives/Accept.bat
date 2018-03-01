coopcmd %1 -c:"All_AcceptSynch"
if errorlevel 1 goto error
goto end

:error
echo Error accepting script
pause

:end

