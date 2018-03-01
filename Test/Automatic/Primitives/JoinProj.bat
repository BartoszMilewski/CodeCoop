co-op.exe -Project_Join project:%1 root:%2 recipient:%3 user:"tester" email:%4
if errorlevel 1 goto error
goto end

:error
echo Error joining project
pause

:end
