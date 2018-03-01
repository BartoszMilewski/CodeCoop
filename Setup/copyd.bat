del bind\*.* /q
del pdb-d\*.* /q

copy ..\co-op\Debug\co-op.exe bind
copy ..\Differ\Debug\Differ.exe bind
copy ..\Dispatcher\Debug\Dispatcher.exe bind
copy ..\SccDll\Debug\SccDll.dll bind
copy ..\Setup\Install\Debug\CoopSetup.exe bind
copy ..\Setup\Uninstall\Debug\Uninstall.exe bind
copy ..\FTPApplet\Debug\FTPApplet.exe bind
copy ..\CmdLine\Unznc\Debug\Unznc.exe bind
copy ..\CmdLine\Diagnostics\Debug\Diagnostics.exe bind
copy ..\CmdLine\DefectFromAll\Debug\DefectFromAll.exe bind
copy "..\CAB file SDK\bin\cabarc.exe" bind
copy SCC.reg bind
copy znc.ico bind
copy "Wiki\wiki.css" bind
copy "Wiki\template.html" bind
copy "Wiki\ErrExists.wiki" bind
copy "Wiki\AddRecord.wiki" bind
copy "Wiki\Forms Help.wiki" bind
copy "Wiki\Help.wiki" bind
copy "Wiki\FAQ.wiki" bind
copy "Wiki\index.wiki" bind
copy "Wiki\SQWiki Help.wiki" bind
copy "Wiki\ToDoList.wiki" bind
copy "Wiki\1.wiki" bind
copy "Wiki\2.wiki" bind
copy "Wiki\Trip.wiki" bind
copy "Wiki\template.wiki" bind
copy "Wiki\sunset.jpg" bind

copy ..\Setup\Help\CodeCoop.chm bind
copy "..\Borland Integration\borland.txt" bind

copy "Beyond Compare\BCMerge.exe" bind
copy "Beyond Compare\BCMerge.chm" bind

copy ReleaseNotes.txt bind
copy CoopVersion.xml bind

copy ..\Setup\binr\borland5.bpl bind
copy ..\Setup\binr\borland6.bpl bind
copy ..\Setup\binr\borland7.bpl bind

copy ..\co-op\Debug\co-op.pdb pdb-d
copy ..\Differ\Debug\Differ.pdb pdb-d
copy ..\Dispatcher\Debug\Dispatcher.pdb pdb-d
copy ..\SccDll\Debug\SccDll.pdb pdb-d
copy ..\Setup\Install\Debug\CoopSetup.pdb pdb-d
copy ..\Setup\Uninstall\Debug\Uninstall.pdb pdb-d
copy ..\CmdLine\Unznc\Debug\Unznc.pdb pdb-d
copy ..\CmdLine\Diagnostics\Debug\Diagnostics.pdb pdb-d

pause
