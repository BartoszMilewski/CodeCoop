del binb\*.* /q
del pdb-b\*.* /q

copy "..\co-op\Beta Lite\co-op.exe" binb
copy "..\Differ\Beta Lite\Differ.exe" binb
copy "..\Dispatcher\Beta Lite\Dispatcher.exe" binb
copy "..\SccDll\Beta Lite\SccDll.dll" binb
copy "..\Setup\Install\Beta Lite\CoopSetup.exe" binb
copy "..\Setup\Uninstall\Beta Lite\Uninstall.exe" binb
copy ..\CmdLine\Unznc\Debug\Unznc.exe binb
copy ..\CmdLine\Diagnostics\Debug\Diagnostics.exe binb
copy ..\CmdLine\DefectFromAll\Debug\DefectFromAll.exe binb
copy SCC.reg binb
copy znc.ico binb
copy "Wiki\wiki.css" binb
copy "Wiki\template.html" binb
copy "Wiki\ErrExists.wiki" binb
copy "Wiki\AddRecord.wiki" binb
copy "Wiki\Forms Help.wiki" binb
copy "Wiki\Help.wiki" binb
copy "Wiki\FAQ.wiki" binb
copy "Wiki\index.wiki" binb
copy "Wiki\SQWiki Help.wiki" binb
copy "Wiki\ToDoList.wiki" binb
copy "Wiki\1.wiki" binb
copy "Wiki\2.wiki" binb
copy "Wiki\Trip.wiki" binb
copy "Wiki\template.wiki" binb
copy "Wiki\sunset.jpg" binb

copy ..\Setup\Help\CodeCoop.chm binb
copy "..\Borland Integration\borland.txt" binb

copy "Beyond Compare\BCMerge.exe" binb
copy "Beyond Compare\BCMerge.chm" binb

copy ReleaseNotes.txt binb
copy CoopVersion.xml binb

copy ..\Setup\binr\borland5.bpl binb
copy ..\Setup\binr\borland6.bpl binb
copy ..\Setup\binr\borland7.bpl binb

copy "..\co-op\Beta Lite\co-op.pdb" pdb-b
copy "..\Differ\Beta Lite\Differ.pdb" pdb-b
copy "..\Dispatcher\Beta Lite\Dispatcher.pdb" pdb-b
copy "..\SccDll\Beta Lite\SccDll.pdb" pdb-b
copy "..\Setup\Install\Beta Lite\CoopSetup.pdb" pdb-b
copy "..\Setup\Uninstall\Beta Lite\Uninstall.pdb" pdb-b
copy ..\CmdLine\Unznc\Debug\Unznc.pdb pdb-b
copy ..\CmdLine\Diagnostics\Debug\Diagnostics.pdb pdb-b

pause
