del bind\*.* /q

copy ..\Setup\Install\Debug\CoopSetup.exe bind
copy CmdLineTools.cfg bind

copy addfile\debug\addfile.exe bind
copy allcoopcmd\debug\allcoopcmd.exe bind
copy checkin\debug\checkin.exe bind
copy out\debug\checkout.exe bind
copy coopcmd\debug\coopcmd.exe bind
copy deliver\debug\deliver.exe bind
copy export\debug\export.exe bind
copy projectstatus\debug\projectstatus.exe bind
copy removefile\debug\removefile.exe bind
copy report\debug\report.exe bind
copy restore\debug\restore.exe bind
copy status\debug\status.exe bind
copy synch\debug\synch.exe bind
copy uncheckout\debug\uncheckout.exe bind
copy unznc\debug\unznc.exe bind
copy versionid\debug\versionid.exe bind
copy label\debug\versionlabel.exe bind
copy visit\debug\visitproject.exe bind
copy coopbackup\debug\startcoopbackup.exe bind

copy ReadMe.txt bind

copy CoopVars.bat bind

pause

