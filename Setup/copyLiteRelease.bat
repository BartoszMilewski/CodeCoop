del binlr\*.* /q
del pdb-r\*.* /q

copy "..\co-op\Release Lite\co-op.exe" binlr
copy "..\Differ\Release Lite\Differ.exe" binlr
copy "..\Dispatcher\Release Lite\Dispatcher.exe" binlr
copy "..\SccDll\Release Lite\SccDll.dll" binlr
copy "..\Setup\Install\Release Lite\CoopSetup.exe" binlr
copy "..\Setup\Uninstall\Release Lite\Uninstall.exe" binlr
copy ..\CmdLine\Unznc\Release\Unznc.exe binlr
copy ..\CmdLine\Diagnostics\Release\Diagnostics.exe binlr
copy ..\CmdLine\DefectFromAll\Release\DefectFromAll.exe binlr
copy SCC.reg binlr
copy znc.ico binlr
copy "Wiki\wiki.css" binlr
copy "Wiki\template.html" binlr
copy "Wiki\ErrExists.wiki" binlr
copy "Wiki\AddRecord.wiki" binlr
copy "Wiki\Forms Help.wiki" binlr
copy "Wiki\Help.wiki" binlr
copy "Wiki\FAQ.wiki" binlr
copy "Wiki\index.wiki" binlr
copy "Wiki\SQWiki Help.wiki" binlr
copy "Wiki\ToDoList.wiki" binlr
copy "Wiki\1.wiki" binlr
copy "Wiki\2.wiki" binlr
copy "Wiki\Trip.wiki" binlr
copy "Wiki\template.wiki" binlr
copy "Wiki\sunset.jpg" binlr

copy ..\Setup\Help\CodeCoop.chm binlr
copy "..\Borland Integration\borland.txt" binlr

copy "Beyond Compare\BCMerge.exe" binlr
copy "Beyond Compare\BCMerge.chm" binlr

copy ReleaseNotes.txt binlr
copy CoopVersion.xml binlr

pause
