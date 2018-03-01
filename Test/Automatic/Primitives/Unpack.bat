coopcmd %1 -c:"All_Synch"
if errorlevel 1 goto error
goto end

:error
echo Error unpacking script
pause

:end

