coopcmd %1 -c:"Project_Defect kind:Tree"
if errorlevel 1 goto error
goto end

:error
echo Error defecting project
pause

:end
