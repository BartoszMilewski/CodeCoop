echo Sleeping...
sleep %2
coopcmd %1 -c:"All_Synch"
if errorlevel 1 goto error
goto end

:error
echo Error synching project
pause

:end

