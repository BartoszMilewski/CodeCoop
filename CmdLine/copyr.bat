del binr\*.* /q

copy ..\Setup\Install\Release\CoopSetup.exe binr
copy CmdLineTools.cfg binr

copy addfile\release\addfile.exe binr
copy allcoopcmd\release\allcoopcmd.exe binr
copy checkin\release\checkin.exe binr
copy out\release\checkout.exe binr
copy coopcmd\release\coopcmd.exe binr
copy deliver\release\deliver.exe binr
copy export\release\export.exe binr
copy projectstatus\release\projectstatus.exe binr
copy removefile\release\removefile.exe binr
copy report\release\report.exe binr
copy restore\release\restore.exe binr
copy status\release\status.exe binr
copy synch\release\synch.exe binr
copy uncheckout\release\uncheckout.exe binr
copy unznc\release\unznc.exe binr
copy versionid\release\versionid.exe binr
copy label\release\versionlabel.exe binr
copy visit\release\visitproject.exe binr
copy coopbackup\release\startcoopbackup.exe binr

copy ReadMe.txt binr

copy CoopVars.bat binr

pause
