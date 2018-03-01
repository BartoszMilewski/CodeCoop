co-op.exe -Project_New project:%1 root:%2 user:"tester" email:%3
if errorlevel 1 goto error
goto end

:error
echo Error creating project
pause

:end

