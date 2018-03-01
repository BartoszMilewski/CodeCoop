del binr\*.* /q
del pdb-r\*.* /q

copy ..\co-op\Release\co-op.exe binr
copy ..\Differ\Release\Differ.exe binr
copy ..\Dispatcher\Release\Dispatcher.exe binr
copy ..\SccDll\Release\SccDll.dll binr
copy ..\Setup\Install\Release\CoopSetup.exe binr
copy ..\Setup\Uninstall\Release\Uninstall.exe binr
copy ..\FTPApplet\Release\FTPApplet.exe binr
copy ..\CmdLine\Unznc\Release\Unznc.exe binr
copy ..\CmdLine\Diagnostics\Release\Diagnostics.exe binr
copy ..\CmdLine\DefectFromAll\Release\DefectFromAll.exe binr
copy "..\CAB file SDK\bin\cabarc.exe" binr
copy SCC.reg binr
copy znc.ico binr
copy "Wiki\wiki.css" binr
copy "Wiki\template.html" binr
copy "Wiki\ErrExists.wiki" binr
copy "Wiki\AddRecord.wiki" binr
copy "Wiki\Forms Help.wiki" binr
copy "Wiki\Help.wiki" binr
copy "Wiki\FAQ.wiki" binr
copy "Wiki\index.wiki" binr
copy "Wiki\SQWiki Help.wiki" binr
copy "Wiki\ToDoList.wiki" binr
copy "Wiki\1.wiki" binr
copy "Wiki\2.wiki" binr
copy "Wiki\Trip.wiki" binr
copy "Wiki\template.wiki" binr
copy "Wiki\sunset.jpg" binr

copy ..\Setup\Help\CodeCoop.chm binr
copy "..\Borland Integration\borland.txt" binr

copy "Beyond Compare\BCMerge.exe" binr
copy "Beyond Compare\BCMerge.chm" binr

copy ReleaseNotes.txt binr
copy CoopVersion.xml binr

copy ..\co-op\Release\co-op.pdb pdb-r
copy ..\Differ\Release\Differ.pdb pdb-r
copy ..\Dispatcher\Release\Dispatcher.pdb pdb-r
copy ..\SccDll\Release\SccDll.pdb pdb-r
copy ..\Setup\Install\Release\CoopSetup.pdb pdb-r
copy ..\Setup\Uninstall\Release\Uninstall.pdb pdb-r
copy ..\CmdLine\Unznc\Release\Unznc.pdb pdb-r
copy ..\CmdLine\Diagnostics\Release\Diagnostics.pdb pdb-r
pause
